#include <Application.hpp>
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
#include <Urho3D/Physics/PhysicsWorld.h>

#include "AppState.hpp"
#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"

namespace UrFelt
{

const char* PHYSICS_CATEGORY = "Physics";
const char* SUBSYSTEM_CATEGORY = "Subsystem";


Application::Application (Urho3D::Context* context) : Urho3D::Application(context),
	m_psurface(), m_quit(false)
{
	context_->RegisterSubsystem(new Urho3D::LuaScript(context_));
}

Application::~Application ()
{
	m_quit = true;
	if (m_thread_updater.joinable())
		m_thread_updater.join();
}

void Application::Stop()
{
	m_plua.reset();
}

Urho3D::String Application::GetTypeNameStatic()
{
	return "UrFelt";
}

void Application::Setup()
{
	using namespace Urho3D;
	engineParameters_["FullScreen"]=false;
	engineParameters_["WindowWidth"]=1280;
	engineParameters_["WindowHeight"]=720;
	engineParameters_["WindowResizable"]=true;
	//engineParameters_["ResourcePaths"] = "Data;CoreData;vendor/share/lua/5.1";

	context_->RegisterSubsystem(this);
	context_->RegisterFactory<UrSurfaceCollisionShape>(PHYSICS_CATEGORY);

	PhysicsWorld::config.collisionConfig_ = new btFeltCollisionConfiguration();


	LuaScript* lua = context_->GetSubsystem<LuaScript>();

	m_plua = std::make_unique<sol::state_view>(lua->GetState());

	sol::table lua_UrFelt = m_plua->create_named_table("UrFelt");

	lua_UrFelt.new_usertype<UrSurface::IsoGrid>(
		"IsoGrid"
	);

	lua_UrFelt.new_usertype<UrSurface>(
		"UrSurface",
		sol::constructors<UrSurface(const Felt::Vec3i&, const Felt::Vec3i&, Node*)>(),
		"seed", &UrSurface::seed,
		"invalidate", &UrSurface::invalidate,
		"polygonise", &UrSurface::polygonise,
		"flush", &UrSurface::flush,

		"update", [](UrSurface& self, sol::function fn_) {
			self.update([&fn_](const Felt::Vec3i& pos_, const UrSurface::IsoGrid& isogrid_) {
				Urho3D::IntVector3 vpos_ = reinterpret_cast<const Urho3D::IntVector3&>(pos_);
				Felt::Distance dist = fn_(vpos_, isogrid_);
				return dist;
			});
		},

		"enqueue_simple", &UrSurface::enqueue_simple,

		"wake", &UrSurface::wake
	);

//	tolua_UrFelt_open (lua->GetState());
//	m_queue_worker.bind(lua->GetState());
//	m_queue_worker.to_lua("queue_worker");
//	m_queue_main.bind(lua->GetState());
//	m_queue_main.to_lua("queue_main");
//	m_queue_script.bind(lua->GetState());
//	m_queue_script.to_lua("queue_in");
//
//	m_queue_script.lua()->writeVariable(
//		"Ray", "_typeid", &typeid(LuaCppMsg::CopyPtr<Urho3D::Ray>*)
//	);
}


void Application::Start()
{
	using namespace Urho3D;
//	GetSubsystem<FileSystem>();
//	Scene* scene = GetSubsystem<Renderer>()->GetViewport(0)->GetScene();
//	Node* node = scene->CreateChild("PolyGrid");
//	node->SetPosition(Vector3(0, 0, 0));
//	m_psurface = std::make_unique<UrSurface>(
//		Felt::Vec3i(200,100,200), Felt::Vec3i(16,16,16), context_, node
//	);

//	start_worker();

//	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Application, tick));

	LuaScript* lua = context_->GetSubsystem<LuaScript>();
	lua->ExecuteFile("Scripts/main.lua");

//	lua->ExecuteFile("Scripts/UrFelt.lua");
//	lua->ExecuteFunction("Init");
//	lua->ExecuteFunction("InitPhysics");

//	using namespace msm;
//	m_controller->process_event("load"_t);

}


void Application::tick(
	Urho3D::StringHash event_type_, Urho3D::VariantMap& event_data_
) {
	using namespace Urho3D;
	using namespace LuaCppMsg;
	using namespace Messages;

	const float dt = event_data_[Update::P_TIMESTEP].GetFloat();

	if (m_app_state_next)
		m_app_state = std::move(m_app_state_next);

	if (this->m_app_state)
		this->m_app_state->tick(dt);
}


void Application::worker()
{
	using namespace Urho3D;
	using namespace Messages;

	typedef std::chrono::high_resolution_clock Clock;
	typedef std::chrono::duration<float> Seconds;

	auto time_last = Clock::now();
	Seconds time_step;

	while (!m_quit)
	{
		Seconds time_step = Clock::now() - time_last;
		time_last = Clock::now();

		while (const UrQueue::Msg::Opt& msg_exists = m_queue_worker.pop())
		{
			using namespace UrFelt::State::Event;
			const UrQueue::Msg& msg = *msg_exists;
			const MsgType type = (MsgType)msg.get("type").as<float>();

		}

		if (m_worker_state_next.get() != m_worker_state.get())
			m_worker_state = m_worker_state_next;

		if (this->m_worker_state)
			this->m_worker_state->tick(time_step.count());
	}
}


void Application::start_worker ()
{
	m_thread_updater = std::thread(&Application::worker, this);
}

} // UrFelt.

URHO3D_DEFINE_APPLICATION_MAIN(UrFelt::Application)
