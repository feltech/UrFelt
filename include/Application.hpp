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
#include <LuaCppMsg.hpp>

#include "UrSurface.hpp"

extern int tolua_UrFelt_open (lua_State* tolua_S);



namespace UrFelt
{
	namespace Messages
	{
		enum MsgType
		{
			STATE_RUNNING = 1, ACTIVATE_SURFACE, START_ZAP, STOP_ZAP, PERCENT_TOP, PERCENT_BOTTOM,
			MAIN_INIT_DONE, WORKER_INIT_DONE, APP_PAUSE, WORKER_PAUSE
		};
	}


	namespace State
	{
		class AppController;
		class BaseSM;
		class AppSM;
		class WorkerRunningSM;
		class WorkerRunningController;
		class TickBase;
		template <class StateType> class Tick;
	}
	using UrQueue = LuaCppMsg::Queue<
		Urho3D::Ray, LuaCppMsg::CopyPtr<Urho3D::Ray>*, float
	>;

	class Application : public Urho3D::Application
	{
		friend class State::BaseSM;
		friend class State::AppSM;
		friend class State::WorkerRunningSM;
		friend class State::WorkerRunningController;
		template <class StateType> friend class State::Tick;
	public:
		~Application();
		Application(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

	private:
		void Setup();
		void Start();
		void tick(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
		void worker();
		void start_worker();

	private:
		std::unique_ptr<State::AppController>	m_controller;
		std::unique_ptr<State::TickBase>		m_app_state;
		std::unique_ptr<State::TickBase>		m_app_state_next;
		std::shared_ptr<State::TickBase>		m_worker_state;
		std::shared_ptr<State::TickBase>		m_worker_state_next;

		std::unique_ptr<UrSurface>				m_psurface;
		Urho3D::RigidBody* 						m_psurface_body;

		std::thread 				m_thread_updater;
		std::atomic<bool>			m_quit;

		UrQueue	m_queue_script;
		UrQueue	m_queue_worker;
		UrQueue	m_queue_main;

		sol::state_view	m_lua;
	};
}
#endif /* INCLUDE_APPLICATION_HPP_ */
