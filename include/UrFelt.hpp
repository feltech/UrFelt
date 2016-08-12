/*
 * UrFelt.hpp
 *
 *  Created on: 5 Jun 2015
 *      Author: dave
 */

#ifndef INCLUDE_URFELT_HPP_
#define INCLUDE_URFELT_HPP_

#include <memory>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <cstdio>
#include <typeinfo>

#include <boost/coroutine/all.hpp>

#include <Urho3D/Urho3D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/LuaScript/LuaScript.h>
#include <Urho3D/Physics/RigidBody.h>

#include <Felt/Surface.hpp>
#include <LuaCppMsg.hpp>

#include "UrSurface3D.hpp"

extern int tolua_UrFelt_open (lua_State* tolua_S);

#include <boost/msm-lite.hpp>


namespace felt
{
	namespace Messages
	{
		enum MsgType
		{
			STATE_RUNNING = 1, ACTIVATE_SURFACE, START_ZAP, STOP_ZAP, PERCENT_TOP, PERCENT_BOTTOM,
			MAIN_INIT_DONE, WORKER_INIT_DONE
		};
	}

	struct AppSM;
	class AppController;
	class WorkerController;
	template <class StateType> struct WorkerState {};


	class UrFelt : public Urho3D::Application
	{
		friend struct AppSM;
	public:
		~UrFelt();
		UrFelt(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

		void repoly();

	protected:
		void Setup();
		void Start();
		void handle_update(
			Urho3D::StringHash eventType, Urho3D::VariantMap& eventData
		);
		void initialiser();
		void tick(float dt);
		void worker();
		void start_worker();

	protected:
		using UrQueue = LuaCppMsg::Queue<
			Urho3D::Ray, LuaCppMsg::CopyPtr<Urho3D::Ray>*, float
		>;

		std::unique_ptr<AppController>		m_controller;
		std::unique_ptr<WorkerController>	m_worker_controller;
		std::unique_ptr<WorkerState<void>>	m_worker_state;

		UrSurface3D				m_surface;
		Urho3D::RigidBody* 		m_surface_body;
		float					m_time_since_update;
		enum State {
			INIT, INIT_DONE, RUNNING,
			STOPPED, STOP, PAUSE, PAUSED, ZAP, REPOLY
		};

		std::thread 				m_thread_updater;
		State						m_state_main;
		std::atomic<State>			m_state_updater;
		std::atomic<bool>			m_quit;
		std::mutex					m_mutex_updater;
		std::condition_variable		m_cond_updater;

		UrQueue	m_queue_script;
		UrQueue	m_queue_worker;
		UrQueue	m_queue_main;
	};

	namespace msm = boost::msm::lite;


	namespace States
	{
		struct InitSurface{};
		struct Zap{};
	}

	namespace Events
	{
		struct StartZap
		{
			const float amt;
		};
		struct StopZap {};
	}

	struct AppSM
	{
		auto configure() const noexcept
		{
			using namespace msm;
			return msm::make_transition_table(
				*"BOOTSTRAP"_s			+ "load"_t 					= "INIT"_s,
				"INIT"_s				+ "initialised"_t 			= "AWAIT_WORKER_INIT"_s,
				"AWAIT_WORKER_INIT"_s 	+ "worker_initialised"_t 	= "RUNNING"_s,
				"RUNNING"_s 			+ "activate_surface"_t 		/
				[](UrFelt* papp) {
					papp->m_surface_body->Activate();
				},
				"RUNNING"_s				+ "stopped"_t				= X
			);
		}
	};


	class AppController : public msm::sm<AppSM>
	{
	public:
		AppController(UrFelt* app_) : msm::sm<AppSM>(std::move(app_)) {}
	};


	template<>
	class WorkerState<void>
	{
	protected:
		UrFelt* m_papp;
	public:
		virtual void tick(const float dt) {};
	};


	template<>
	class WorkerState<States::Zap> : public WorkerState<void>
	{
	public:
		const FLOAT amt;
	public:
		void tick(const float dt);
	};


	template<>
	class WorkerState<States::InitSurface> : public WorkerState<void>
	{
	protected:
		boost::coroutines::coroutine<>::pull_type initialiser;
		void init_surface_task(boost::coroutines::coroutine<FLOAT>::push_type& sink);
	public:
		void tick(const float dt);
	};


	struct WorkerRunningSM
	{
		auto configure() const noexcept
		{
			using namespace msm;

			state<States::Zap> ZAP;

			return msm::make_transition_table(

				"IDLE"_s(H)	+ event<Events::StartZap>	/
				[](const Events::StartZap& evt) {
					papp->m_worker_state = std::unique_ptr<WorkerState<States::Zap>>(
						new WorkerState<States::Zap>{papp, evt.amt}
					);
				}										=	ZAP,

				ZAP			+ event<StartZap>			/
				[](const Events::StartZap& evt){
					papp->m_worker_state = std::unique_ptr<WorkerState<States::Zap>>(
						new WorkerState<States::Zap>{papp, evt.amt}
					);
				}										=	ZAP,

				ZAP			+ event<StopZap>			/
				[](const Events::StartZap& evt){
					papp->m_worker_state = std::unique_ptr<WorkerState<void>>(
						new WorkerState<void>{}
					);
				}										=	"IDLE"_s,
			);
		}
	};

	struct WorkerSM
	{
		auto configure() const noexcept
		{
			using namespace msm;
			state<sm<WorkerRunningSM>> WORKER_RUNNING;
			state<States::InitSurface> INIT_SURFACE;

			return make_transition_table(
				*"INIT"_s 													/
				[](UrFelt* papp){
					papp->m_worker_state = std::unique_ptr<WorkerState<States::InitSurface>>(
						new WorkerState<States::InitSurface>{papp}
					);
				}															= INIT_SURFACE,

				INIT_SURFACE					+ "surface_initialised"_t	/
				[](UrFelt* papp, const Events::StartZap& evt){
					papp->m_worker_state = std::unique_ptr<WorkerState<void>>(
						new WorkerState<void>{}
					);
				}															= "AWAIT_MAIN_INIT"_s,

				"AWAIT_MAIN_INIT"_s 			+ "initialised"_t 			= WORKER_RUNNING,

				WORKER_RUNNING 					+ "repoly"_t	 			/
				[](UrFelt* papp, const Events::Tick& evt) {
					papp->m_surface.poly.surf()
				},

				WORKER_RUNNING					+ "pause"_t					= "WORKER_PAUSED"_s,
				"WORKER_PAUSED"_s				+ "resume"_t				= WORKER_RUNNING,

				WORKER_RUNNING					+ "stop"_t					= X,
				"WORKER_PAUSED"_s				+ "stop"_t					= X
			);
		}
	};

	class WorkerController : public msm::sm<AppSM>
	{
	public:
		WorkerController(UrFelt* app_) : msm::sm<WorkerSM>(std::move(app_)) {}
	};
}
#endif /* INCLUDE_URFELT_HPP_ */
