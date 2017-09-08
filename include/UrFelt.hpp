#ifndef INCLUDE_URFELT_HPP_
#define INCLUDE_URFELT_HPP_

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

#include <Felt/Surface.hpp>
#include <LuaCppMsg.hpp>

#include "UrSurface3D.hpp"

extern int tolua_UrFelt_open (lua_State* tolua_S);



namespace Felt
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

	class UrFelt : public Urho3D::Application
	{
		friend class Felt::State::BaseSM;
		friend class Felt::State::AppSM;
		friend class Felt::State::WorkerRunningSM;
		friend class Felt::State::WorkerRunningController;
		template <class StateType> friend class Felt::State::Tick;
	public:
		~UrFelt();
		UrFelt(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

	private:
		void Setup();
		void Start();
		void tick(Urho3D::StringHash eventType, Urho3D::VariantMap& eventData);
		void worker();
		void start_worker();

	private:
		std::unique_ptr<Felt::State::AppController>	m_controller;
		std::unique_ptr<Felt::State::TickBase>		m_app_state;
		std::unique_ptr<Felt::State::TickBase>		m_app_state_next;
		std::shared_ptr<Felt::State::TickBase>		m_worker_state;
		std::shared_ptr<Felt::State::TickBase>		m_worker_state_next;

		UrSurface3D				m_surface;
		Urho3D::RigidBody* 		m_surface_body;

		std::thread 				m_thread_updater;
		std::atomic<bool>			m_quit;

		UrQueue	m_queue_script;
		UrQueue	m_queue_worker;
		UrQueue	m_queue_main;
	};
}
#endif /* INCLUDE_URFELT_HPP_ */
