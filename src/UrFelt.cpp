//#define FELT_EXCEPTIONS

#include "UrFelt.hpp"


#include <thread>
#include <chrono>
#include <omp.h>
#include <GL/gl.h>

#include <boost/msm-lite.hpp>

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


void WorkerState<State::InitApp>::tick(const float dt)
{
	if (m_co)
	{
		using namespace Messages;
		const FLOAT frac_done = m_co().get();
		this->m_papp->m_queue_script.push(UrQueue::Map{
			{"type", PERCENT_TOP},
			{"value", (FLOAT)(INT)(100.0 * frac_done)},
			{"label", "Initialising physics"}
		});
	}
	else
	{
		using namespace msm;
		m_papp->m_controller->process_event("app_initialised"_t);
	}
}

void WorkerState<State::InitSurface>::tick(const float dt)
{
	if (m_co)
	{
		using namespace Messages;
		const FLOAT frac_done = m_co().get();
		m_papp->m_queue_script.push(UrQueue::Map{
			{"type", PERCENT_TOP},
			{"value", (FLOAT)(INT)(100.0 * frac_done)},
			{"label", "Initialising surface"}
		});
	}
	else
	{
		using namespace msm;
		m_papp->m_controller->process_event("surface_initialised"_t);
	}
}

void WorkerState<State::Running>::tick(const float dt)
{
	m_time_since_update += dt;
	if (m_time_since_update > 1.0f/30.0f)
	{
		using namespace msm;
		m_papp->m_controller->process_event("update_gpu"_t);
	}
}

void WorkerState<State::UpdateGPU>::tick(const float dt)
{
	using namespace msm;
	m_papp->m_surface.poly().update_gpu();
	m_papp->m_surface.poly().update_end();
	m_papp->m_controller->process_event("resume"_t);
}


template <class StateTypeApp, class StateTypeWorker>
std::function<bool (UrFelt*)> AppSM::is(StateTypeApp state_app_, StateTypeWorker state_worker_)
{
	return [state_app_, state_worker_](UrFelt* papp) {
		return papp->m_controller->is(state_app_, state_worker_);
	};
}

template <class StateTypeApp, class StateTypeWorker>
std::function<bool (UrFelt*)> AppSM::is_not(StateTypeApp state_app_, StateTypeWorker state_worker_)
{
	using namespace msm;
	return [state_app_, state_worker_](UrFelt* papp) {
		return !papp->m_controller->is(state_app_, state_worker_);
	};
}

template <class EventType>
std::function<void (UrFelt*)> AppSM::trigger(EventType event_)
{
	using namespace msm;
	return [event_](UrFelt* papp) {
		papp->m_controller->process_event(event_);;
	};
}

std::function<void (UrFelt*)> AppSM::worker_idleing()
{
	using namespace msm;
	return [](UrFelt* papp) {
		papp->m_worker_state.reset(nullptr);
	};
}

std::function<void (UrFelt*)> AppSM::app_idleing()
{
	using namespace msm;
	return [](UrFelt* papp) {
		papp->m_app_state.reset(nullptr);
	};
}


UrFelt::~UrFelt ()
{
	m_quit = true;
	if (m_thread_updater.joinable())
		m_thread_updater.join();
}


UrFelt::UrFelt (Urho3D::Context* context) : Urho3D::Application(context),
	m_surface(), m_quit(false), m_controller(new AppController(this))
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

	m_surface_body = node->CreateComponent<RigidBody>();
	m_surface_body->SetKinematic(true);
	m_surface_body->SetMass(10000000.0f);
	m_surface_body->SetFriction(1.0f);
	m_surface_body->SetUseGravity(false);
	m_surface_body->SetRestitution(0.0);

	start_worker();

	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(UrFelt, handle_update));

	using namespace msm;
	m_controller->process_event("load"_t);
}


void UrFelt::handle_update(
	Urho3D::StringHash event_type_, Urho3D::VariantMap& event_data_
) {
	using namespace Urho3D;
	using namespace LuaCppMsg;
	using namespace Messages;
	using namespace msm;

	while (UrQueue::Msg::Opt msg_exists = m_queue_main.pop())
	{
		const UrQueue::Msg& msg = *msg_exists;
		const MsgType type = (MsgType)msg.get("type").as<float>();

		switch (type)
		{
		case MsgType::STATE_RUNNING:
			m_controller->process_event("worker_initialised"_t);
			break;
		case MsgType::ACTIVATE_SURFACE:
			m_controller->process_event("activate_surface"_t);
			break;
		}
	}

	const float dt = event_data_[Update::P_TIMESTEP].GetFloat();

	if (this->m_app_state)
		this->m_app_state->tick(dt);
}


void UrFelt::worker()
{
	using namespace Urho3D;
	using namespace Messages;

	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Seconds;

	auto time_last = Clock::now();
	Seconds time_step;

	UINT expand_count = 0;
	const UINT expand_max = 100;

	while (!m_quit)
	{
		Seconds time_step = Clock::now() - time_last;
		time_last = Clock::now();
		while (const UrQueue::Msg::Opt& msg_exists = m_queue_worker.pop())
		{
			const UrQueue::Msg& msg = *msg_exists;
			const MsgType type = (MsgType)msg.get("type").as<float>();

			if (type == MsgType::STOP_ZAP)
			{
				this->m_controller->process_event(Event::StopZap{});
			}
			else if (type == MsgType::START_ZAP)
			{
				const float zap_amount = msg.get("amount").as<float>();
				this->m_controller->process_event(Event::StartZap{zap_amount});
			}
			else if (type == MsgType::STATE_RUNNING)
			{
			}
			else
			{
				std::ostringstream str;
				str << "Invalid message type: " << type;
				throw std::runtime_error(str.str());
			}
		}

		if (this->m_worker_state)
			this->m_worker_state->tick(time_step.count());

	}
}


void UrFelt::start_worker ()
{
	m_thread_updater = std::thread(&UrFelt::worker, this);
}

URHO3D_DEFINE_APPLICATION_MAIN(felt::UrFelt)
