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

WorkerState<State::Running>::tick(const float dt)
{
	m_time_since_update += dt;
	if (m_time_since_update > 1.0f/30.0f)
	{
		m_papp->m_controller->process_event("update_gpu"_t);
	}
}

WorkerState<State::UpdateGPU>::tick(const float dt)
{
	using namespace msm;
	m_papp->m_time_since_update = 0;
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
	m_cond_updater.notify_all();
	if (m_thread_updater.joinable())
		m_thread_updater.join();
}


UrFelt::UrFelt (Urho3D::Context* context) : Urho3D::Application(context),
	m_surface(), m_time_since_update(0), m_state_main(INIT), m_state_updater(STOPPED),
	m_quit(false), m_controller(new AppController(this))
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

void UrFelt::update_gpu()
{
	using namespace msm;
	m_time_since_update = 0;
	m_surface.poly().update_gpu();
	m_surface.poly().update_end();
	m_controller->process_event("resume"_t);
}


void UrFelt::tick(float dt)
{
	// Take the frame time step, which is stored as a float
	float time_step = dt;//event_data_[Update::P_TIMESTEP].GetFloat();
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
}


void UrFelt::worker()
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
			const MsgType type = (MsgType)msg.get("type").as<float>();

			if (type == MsgType::STOP_ZAP)
			{
				is_zapping = false;
			}
			else if (type == MsgType::START_ZAP)
			{
				is_zapping = true;
				zap_ray = Ray(msg.get("ray").as<Urho3D::Ray>());
				zap_amount = msg.get("amount").as<float>();

				m_queue_main.push(UrQueue::Map{
					{ "type", ACTIVATE_SURFACE }
				});
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
						return 0.0f;
					else
						return -1.0f;
				});

				expand_count++;

				m_queue_script.push(UrQueue::Map{
					{"type", MsgType::PERCENT_BOTTOM},
					{"value", (FLOAT)(INT)(100.0 * (FLOAT)expand_count / expand_max)},
					{"label", "Initialising surface"}
				});
			}
			if (expand_count == expand_max)
			{
				m_queue_script.push(UrQueue::Map{
					{"type", MsgType::PERCENT_BOTTOM},
					{"value", -1.0}
				});
				m_queue_script.push(MsgType::WORKER_INIT_DONE);
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


void UrFelt::start_worker ()
{
	m_state_updater = INIT;
	m_thread_updater = std::thread(&UrFelt::worker, this);
}


void UrFelt::repoly ()
{
	URHO3D_LOGINFO("Repolying");
	if (m_state_updater == RUNNING)
		m_state_updater = REPOLY;
	else
		URHO3D_LOGWARNING("Failed to repoly - busy");
}

//	return msm::make_transition_table(
//		*"BOOTSTRAP"_s		+ "load"_t 					= "INIT"_s,
//		"INIT"_s			+ msm::event<Tick> /
//			[](UrFelt* pthis) {
//				pthis->initialiser();
//			},
//		"INIT"_s			+ "initialised"_t 			= "AWAIT_WORKER"_s,
//		"AWAIT_WORKER"_s 	+ "worker_initialised"_t 	= "RUNNING",
//		"RUNNING"_s 		+ msm::event<Tick> /
//			[](UrFelt* pthis, const msm::event<Tick>& evt) {
//				pthis->tick(evt.dt);
//			},
//		"RUNNING"_s 		+ "activate_surface"_t /
//			[](UrFelt* pthis, const msm::event<Tick>& evt) {
//				pthis->m_surface_body->Activate();
//			},
//		"RUNNING"_s			+ "stopped"					= X
//	);

URHO3D_DEFINE_APPLICATION_MAIN(felt::UrFelt)
