#ifndef INCLUDE_APPLICATION_HPP_
#define INCLUDE_APPLICATION_HPP_
#include <Common.hpp>

#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <cstdio>
#include <typeinfo>

#include <Urho3D/Urho3D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/LuaScript/LuaScript.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/IO/Log.h>

#include <sol.hpp>

#include <Felt/Surface.hpp>

#include "UrSurface.hpp"


extern int tolua_UrFelt_open (lua_State* tolua_S);


namespace UrFelt
{
	class Application : public Urho3D::Application
	{
	public:
		Application(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

	private:
		void Setup();
		void Start();
		void Stop();

	private:
		std::unique_ptr<sol::state_view>	m_plua;
	};
}
#endif /* INCLUDE_APPLICATION_HPP_ */
