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

#include "AppState.hpp"
#include "FeltCollisionShape.hpp"
#include "btFeltCollisionConfiguration.hpp"

using namespace felt;


const char* PHYSICS_CATEGORY = "Physics";
const char* SUBSYSTEM_CATEGORY = "Subsystem";


UrFelt::~UrFelt ()
{
	m_quit = true;
	if (m_thread_updater.joinable())
		m_thread_updater.join();
}


UrFelt::UrFelt (Urho3D::Context* context) : Urho3D::Application(context),
	m_surface(), m_quit(false), m_controller(new State::AppController(this))
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

	PhysicsWorld::config.collisionConfig_ = new btFeltCollisionConfiguration();

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

	m_surface.init(context_, node, Vec3u(200,100,200), Vec3u(16,16,16));

	m_surface_body = node->CreateComponent<RigidBody>();
	m_surface_body->SetKinematic(true);
	m_surface_body->SetMass(10000000.0f);
	m_surface_body->SetFriction(1.0f);
	m_surface_body->SetUseGravity(false);
	m_surface_body->SetRestitution(0.0);

	start_worker();

	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(UrFelt, tick));

	using namespace msm;
	m_controller->process_event("load"_t);
}


void UrFelt::tick(
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
		case MsgType::ACTIVATE_SURFACE:
			m_controller->process_event("activate_surface"_t);
			break;
		default:
			std::ostringstream str;
			str << "Invalid message type: " << type;
			throw std::runtime_error(str.str());
		}
	}

	const float dt = event_data_[Update::P_TIMESTEP].GetFloat();

	if (m_app_state_next)
		m_app_state = std::move(m_app_state_next);

	if (this->m_app_state)
		this->m_app_state->tick(dt);
}


void UrFelt::worker()
{
	using namespace Urho3D;
	using namespace Messages;
	using namespace msm;

	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Seconds;

	auto time_last = Clock::now();
	Seconds time_step;

	while (!m_quit)
	{
		Seconds time_step = Clock::now() - time_last;
		time_last = Clock::now();

		if (this->m_controller->is(State::AppController::WORKER_RUNNING))
			while (const UrQueue::Msg::Opt& msg_exists = m_queue_worker.pop())
			{
				using namespace felt::State::Event;
				const UrQueue::Msg& msg = *msg_exists;
				const MsgType type = (MsgType)msg.get("type").as<float>();
				switch (type)
				{
				case MsgType::STOP_ZAP:
				{
					this->m_controller->process_event(StopZap{});
					break;
				}
				case MsgType::START_ZAP:
				{
					const float zap_amount = msg.get("amount").as<float>();
					this->m_controller->process_event(StartZap{zap_amount});
					break;
				}
				default:
				{
					std::ostringstream str;
					str << "Invalid message type: " << type;
					throw std::runtime_error(str.str());
				}
				}
			}

		if (m_worker_state_next.get() != m_worker_state.get())
			m_worker_state = m_worker_state_next;

		if (this->m_worker_state)
			this->m_worker_state->tick(time_step.count());
	}
}


void UrFelt::start_worker ()
{
	m_thread_updater = std::thread(&UrFelt::worker, this);
}

URHO3D_DEFINE_APPLICATION_MAIN(felt::UrFelt)
