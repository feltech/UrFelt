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


Urho3D::String Application::GetTypeNameStatic()
{
	return "UrFelt";
}


Application::Application (Urho3D::Context* context) :
	Urho3D::Application(context)
{
	context_->RegisterSubsystem(new Urho3D::LuaScript(context_));
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

	m_plua->set_function("now", []{
		return std::chrono::system_clock::now().time_since_epoch() /
		    std::chrono::milliseconds(1);
	});

	sol::table lua_UrFelt = m_plua->create_named_table("UrFelt");

	UrSurface::to_lua(lua_UrFelt);
}


void Application::Start()
{
	using namespace Urho3D;
//	SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Application, tick));

	LuaScript* lua = context_->GetSubsystem<LuaScript>();
	lua->ExecuteFile("Scripts/main.lua");
}


void Application::Stop()
{
	m_plua.reset();
}

} // UrFelt.

URHO3D_DEFINE_APPLICATION_MAIN(UrFelt::Application)
