//#define FELT_EXCEPTIONS

#include "UrFelt.hpp"

#include <thread>
#include <chrono>
#include <omp.h>
#include <GL/gl.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/LuaScript/LuaScript.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/Camera.h>

#include "UrPolyGrid3D.hpp"


using namespace felt;

UrFelt::~UrFelt ()
{
	m_state_updater = STOP;
	if (m_thread_updater.joinable())
		m_thread_updater.join();
}


UrFelt::UrFelt (Urho3D::Context* context) : Urho3D::Application(context),
	m_surface(), m_time_since_update(0), m_state_updater(STOPPED),
	m_zap(Zapper())
{
	context_->RegisterSubsystem(new Urho3D::LuaScript(context_));
}

Urho3D::String UrFelt::GetTypeNameStatic()
{
	return "UrFelt";
}

void UrFelt::Setup()
{
	using namespace Urho3D;
	engineParameters_["FullScreen"]=false;
	engineParameters_["WindowWidth"]=1280;
	engineParameters_["WindowHeight"]=720;
	engineParameters_["WindowResizable"]=true;

	context_->RegisterSubsystem(this);
	LuaScript* lua = context_->GetSubsystem<LuaScript>();
	tolua_UrFelt_open (lua->GetState());
}


void UrFelt::Start()
{
	using namespace Urho3D;
	LuaScript* lua = context_->GetSubsystem<LuaScript>();
	lua->ExecuteFile("Scripts/UrFelt.lua");
	lua->ExecuteFunction("Init");

	Scene* scene = GetSubsystem<Renderer>()->GetViewport(0)->GetScene();
	Node* node = scene->CreateChild("PolyGrid");
	node->SetPosition(Vector3(0.0f, 0.0f, 0.0f));

	m_surface.init(context_, node, Vec3u(200,200,200), Vec3u(16,16,16));
	m_surface.seed(Vec3i(0,0,0));
	m_surface.update([](auto& pos, auto& phi)->FLOAT {
		return -1.0f;
	});

	start_updater();

	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(UrFelt, handle_update));
}

void UrFelt::handle_update(
	Urho3D::StringHash event_type_, Urho3D::VariantMap& event_data_
) {
	using namespace Urho3D;
	// Take the frame time step, which is stored as a float
	float time_step = event_data_[Update::P_TIMESTEP].GetFloat();
	m_time_since_update += time_step;

	if (m_time_since_update > 1.0f/30.0f)
	{
		switch (m_state_updater)
		{
		case RUNNING:
			m_state_updater = PAUSE;
			break;
		case PAUSED:
			m_time_since_update = 0;
			m_surface.poly().update_gpu();
			m_surface.poly().update_end();
			m_cond_updater.notify_all();
			break;
		}
	}
}


void UrFelt::updater()
{
	using namespace Urho3D;
	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Seconds;

	std::unique_lock<std::mutex> lock(m_mutex_updater);

	auto time_last = Clock::now();
	Seconds time_step;
	float ftime_step;

	while (m_state_updater != STOP)
	{
		Zapper zap;
		switch (m_state_updater)
		{
		case RUNNING:
			time_step =  (Clock::now() - time_last);
			if (time_step.count() < 3)
			{
				m_surface.update([](auto& pos, auto& phi)->FLOAT {
					using namespace felt;
					const Vec3f grad = phi.gradE(pos);
					const FLOAT mag = grad.blueNorm();
					const FLOAT curv = phi.curv(pos);
					const FLOAT delta = -0.5f * mag * (1.0f - 0.6f * curv);

					return delta;
				});
			}

			zap = m_zap.load();
			if (zap.amount != 0)
			{
				for (UINT i = 0; i < 1; i++)
				{
					m_surface.update_start();
					FLOAT leftover = m_surface.dphi_gauss<4>(
						Vec3f(zap.pos[0], zap.pos[1], zap.pos[2]),
						Vec3f(zap.dir[0], zap.dir[1], zap.dir[2]),
						zap.amount, 2.0f
					);
					m_surface.update_end_local();
					m_surface.poly().notify(m_surface);
				}
			}

			break;

		case REPOLY:
			m_surface.poly().surf(m_surface);
			URHO3D_LOGINFO("Repolyed");
			m_state_updater = RUNNING;
			break;
		case PAUSE:
			m_surface.poly().poly_cubes(m_surface);
			m_state_updater = PAUSED;
			m_cond_updater.wait(lock);
			m_state_updater = RUNNING;
			break;
		}
	}

	lock.unlock();
	m_state_updater = STOPPED;
}


void UrFelt::start_updater ()
{
	m_state_updater = RUNNING;
	m_thread_updater = std::thread(&UrFelt::updater, this);
}


void UrFelt::zap (const Urho3D::Ray& ray, const float& amount)
{
	using namespace Urho3D;

	const Camera* camera = (
		GetSubsystem<Renderer>()->GetViewport(0)->GetCamera()
	);

//	const Ray& ray = camera->GetScreenRay(0.5, 0.5);

//	LOGINFO(
//		String("Zapping: ") + ray.origin_.ToString()
//		+ String(" in direction ") + ray.direction_.ToString()
//	);
	Zapper zap;
	zap.pos[0] = ray.origin_.x_;
	zap.pos[1] = ray.origin_.y_;
	zap.pos[2] = ray.origin_.z_;
	zap.dir[0] = ray.direction_.x_;
	zap.dir[1] = ray.direction_.y_;
	zap.dir[2] = ray.direction_.z_;
	zap.amount = amount;
	m_zap = zap;
}


void UrFelt::repoly ()
{
	URHO3D_LOGINFO("Repolying");
	if (m_state_updater == RUNNING)
		m_state_updater = REPOLY;
	else
		URHO3D_LOGWARNING("Failed to repoly - busy");
}

URHO3D_DEFINE_APPLICATION_MAIN(felt::UrFelt)
