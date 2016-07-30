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
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Physics/PhysicsWorld.h>

#include "FeltCollisionShape.hpp"
#include "btFeltCollisionConfiguration.hpp"

using namespace felt;

const char* PHYSICS_CATEGORY = "Physics";
const char* SUBSYSTEM_CATEGORY = "Subsystem";


UrFelt::~UrFelt ()
{
	m_quit = true;
	m_cond_updater.notify_all();
	if (m_thread_updater.joinable())
		m_thread_updater.join();
}


UrFelt::UrFelt (Urho3D::Context* context) : Urho3D::Application(context),
	m_surface(), m_time_since_update(0), m_state_main(INIT), m_state_updater(STOPPED),
	m_quit(false)
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
	context_->RegisterFactory<FeltCollisionShape>(PHYSICS_CATEGORY);

	PhysicsWorld::config.collisionConfig = new btFeltCollisionConfiguration();

	LuaScript* lua = context_->GetSubsystem<LuaScript>();
	tolua_UrFelt_open (lua->GetState());

	m_queue_worker.bind(lua->GetState());
	m_queue_worker.to_lua("queue_worker");
	m_queue_main.bind(lua->GetState());
	m_queue_main.to_lua("queue_main");
	m_queue_script.bind(lua->GetState());
	m_queue_script.to_lua("queue_in");

	m_queue_script.lua()->writeVariable(
		"Ray", "_typeid", &typeid(LuaCppMsg::CopyPtr<Urho3D::Ray>*)
	);
}


void UrFelt::Start()
{
	using namespace Urho3D;
	LuaScript* lua = context_->GetSubsystem<LuaScript>();
	lua->ExecuteFile("Scripts/UrFelt.lua");
	lua->ExecuteFunction("Init");

	Scene* scene = GetSubsystem<Renderer>()->GetViewport(0)->GetScene();

	lua->ExecuteFunction("InitPhysics");

	Node* node = scene->CreateChild("PolyGrid");
	node->SetPosition(Vector3(0, 0, 0));

	m_surface.init(context_, node, Vec3u(100,100,100), Vec3u(16,16,16));
	m_surface.seed(Vec3i(0,0,0));

	for (UINT i = 0; i < 2; i++)
		m_surface.update([](auto& pos, auto& phi)->FLOAT {
			return -1.0f;
		});

	m_surface_body = node->CreateComponent<RigidBody>();
	m_surface_body->SetKinematic(true);
	m_surface_body->SetMass(10000000.0f);
	m_surface_body->SetFriction(1.0f);
	m_surface_body->SetUseGravity(false);
	m_surface_body->SetRestitution(0.0);

	start_updater();

	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(UrFelt, handle_update));
}

void UrFelt::handle_update(
	Urho3D::StringHash event_type_, Urho3D::VariantMap& event_data_
) {
	using namespace Urho3D;
	using namespace LuaCppMsg;
	using namespace Messages;

	switch (m_state_main)
	{
	case INIT:
	{
		FLOAT frac_physics_inited = m_surface.init_physics_chunk();
		if (frac_physics_inited == 1)
		{
			m_queue_script.push(UrQueue::Msg(UrQueue::Map{
				{"type", UrQueue::Num(MsgType::PERCENT_TOP)},
				{"value", -1.0}
			}));
			m_queue_script.push(UrQueue::Msg(UrQueue::Num(MsgType::MAIN_INIT_DONE)));
			m_state_main = INIT_DONE;
		}
		else
		{
			m_queue_script.push(UrQueue::Msg(UrQueue::Map{
				{"type", UrQueue::Num(MsgType::PERCENT_TOP)},
				{"value", (FLOAT)(INT)(100.0 * frac_physics_inited)},
				{"label", "Initialising physics"}
			}));
		}
		break;
	}

	case INIT_DONE:
		while (UrQueue::Msg::Opt msg_exists = m_queue_main.pop())
		{
			const UrQueue::Msg& msg = *msg_exists;
			const MsgType type = (MsgType)msg.get("type").as<UrQueue::Num>();

			if (type == MsgType::STATE_RUNNING)
				m_state_main = RUNNING;
		}
		break;

	case RUNNING:
	{
		while (UrQueue::Msg::Opt msg_exists = m_queue_main.pop())
		{
			const UrQueue::Msg& msg = *msg_exists;
			const MsgType type = (MsgType)msg.get("type").as<UrQueue::Num>();
			if (type == MsgType::ACTIVATE_SURFACE)
			{
				m_surface_body->Activate();
			}
			else
			{
				std::ostringstream str;
				str << "Invalid message type: " << type;
				throw std::runtime_error(str.str());
			}
		}

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
		break;
	}
	}

}


void UrFelt::updater()
{
	using namespace Urho3D;
	using namespace Messages;

	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Seconds;

	std::unique_lock<std::mutex> lock(m_mutex_updater);

	auto time_last = Clock::now();
	Seconds time_step;

	UINT expand_count = 0;
	const UINT expand_max = 100;

	bool is_physics_done = false;

	bool is_zapping = false;
	Ray zap_ray;
	float zap_amount;

	while (!m_quit)
	{
		while (const UrQueue::Msg::Opt& msg_exists = m_queue_worker.pop())
		{
			const UrQueue::Msg& msg = *msg_exists;
			const MsgType type = (MsgType)msg.get("type").as<UrQueue::Num>();

			if (type == MsgType::STOP_ZAP)
			{
				is_zapping = false;
			}
			else if (type == MsgType::START_ZAP)
			{
				is_zapping = true;
				zap_ray = Ray(msg.get("ray").as<Urho3D::Ray>());
				zap_amount = msg.get("amount").as<UrQueue::Num>();

				m_queue_main.push(UrQueue::Msg(UrQueue::Map{
					{ "type", UrQueue::Num(ACTIVATE_SURFACE) }
				}));
			}
			else if (type == MsgType::STATE_RUNNING)
			{
				m_state_updater = RUNNING;
			}
			else
			{
				std::ostringstream str;
				str << "Invalid message type: " << type;
				throw std::runtime_error(str.str());
			}
		}

		switch (m_state_updater)
		{

		case INIT:
			if (expand_count < expand_max)
			{
				m_surface.update([](auto& pos, auto& phi)->FLOAT {
					using namespace felt;
					if (std::abs(pos(1)) > 1)
						return 0;
					else
						return -1;
				});

				expand_count++;

				m_queue_script.push(UrQueue::Msg(UrQueue::Map{
					{"type", UrQueue::Num(MsgType::PERCENT_BOTTOM)},
					{"value", (FLOAT)(INT)(100.0 * (FLOAT)expand_count / expand_max)},
					{"label", "Initialising surface"}
				}));
			}
			if (expand_count == expand_max)
			{
				m_queue_script.push(UrQueue::Msg(UrQueue::Map{
					{"type", UrQueue::Num(MsgType::PERCENT_BOTTOM)},
					{"value", -1.0}
				}));
				m_queue_script.push(UrQueue::Msg(UrQueue::Num(MsgType::WORKER_INIT_DONE)));
				m_state_updater = INIT_DONE;
			}
			break;

		case INIT_DONE:
			break;

		case RUNNING:
			if (is_zapping)
			{
				m_surface.update_start();
				FLOAT leftover = m_surface.delta_gauss<4>(
					reinterpret_cast<Vec3f&>(zap_ray.origin_),
					reinterpret_cast<Vec3f&>(zap_ray.direction_),
					zap_amount, 2.0f
				);
				m_surface.update_end_local();
				m_surface.poly().notify(m_surface);
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
	m_state_updater = INIT;
	m_thread_updater = std::thread(&UrFelt::updater, this);
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
