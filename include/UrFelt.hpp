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
#include <Urho3D/Input/Input.h>
#include <Urho3D/Graphics/Camera.h>

#include <Felt/Surface.hpp>
#include <LuaCppMsg.hpp>

#include "UrSurface3D.hpp"

extern int tolua_UrFelt_open (lua_State* tolua_S);


template <class SM, class TEvent>
void log_process_event(const TEvent&) {
  printf("[%s][process_event] %s\n", typeid(SM).name(), typeid(TEvent).name());
}

template <class SM, class TGuard, class TEvent>
void log_guard(const TGuard&, const TEvent&, bool result) {
  printf("[%s][guard] %s %s %s\n", typeid(SM).name(), typeid(TGuard).name(), typeid(TEvent).name(),
         (result ? "[OK]" : "[Reject]"));
}

template <class SM, class TAction, class TEvent>
void log_action(const TAction&, const TEvent&) {
  printf("[%s][action] %s %s\n", typeid(SM).name(), typeid(TAction).name(), typeid(TEvent).name());
}

template <class SM, class TSrcState, class TDstState>
void log_state_change(const TSrcState& src, const TDstState& dst) {
  printf("[%s][transition] %s -> %s\n", typeid(SM).name(), src.c_str(), dst.c_str());
}

#define BOOST_MSM_LITE_LOG(T, SM, ...) log_##T<SM>(__VA_ARGS__)
#define BOOST_MSM_LITE_THREAD_SAFE
#include <boost/msm-lite.hpp>


namespace felt
{
	namespace Messages
	{
		enum MsgType
		{
			STATE_RUNNING = 1, ACTIVATE_SURFACE, START_ZAP, STOP_ZAP, PERCENT_TOP, PERCENT_BOTTOM,
			MAIN_INIT_DONE, WORKER_INIT_DONE, APP_PAUSE, WORKER_PAUSE
		};
	}


	struct AppSM;
	class AppController;
	class WorkerController;
	class WorkerSM;
	class WorkerRunningSM;
	template <class StateType> class WorkerState {};

	using UrQueue = LuaCppMsg::Queue<
		Urho3D::Ray, LuaCppMsg::CopyPtr<Urho3D::Ray>*, float
	>;

	class UrFelt : public Urho3D::Application
	{
		friend struct AppSM;
		friend struct WorkerSM;
		friend struct WorkerRunningSM;
		template <class StateType> friend class WorkerState;
	public:
		~UrFelt();
		UrFelt(Urho3D::Context* context);

		static Urho3D::String GetTypeNameStatic();

	private:
		void Setup();
		void Start();
		void handle_update(
			Urho3D::StringHash eventType, Urho3D::VariantMap& eventData
		);
		void tick(float dt);
		void worker();
		void start_worker();

	private:
		std::unique_ptr<AppController>		m_controller;
		std::unique_ptr<WorkerState<void>>	m_app_state;
		std::unique_ptr<WorkerState<void>>	m_worker_state;
		std::mutex					m_mutex_app_statechange;
		std::condition_variable		m_cond_app_statechange;
		std::mutex					m_mutex_worker_statechange;
		std::condition_variable		m_cond_worker_statechange;

		UrSurface3D				m_surface;
		Urho3D::RigidBody* 		m_surface_body;

		std::thread 				m_thread_updater;
		std::atomic<bool>			m_quit;

		UrQueue	m_queue_script;
		UrQueue	m_queue_worker;
		UrQueue	m_queue_main;
	};

	namespace msm = boost::msm::lite;
	namespace co = boost::coroutines;

	namespace State
	{
		struct InitApp{};
		struct InitSurface{};
		struct Zap{};
		struct Running{};
		struct UpdateGPU{};
	}

	namespace Event
	{
		struct StartZap
		{
			const float amt;
		};
		struct StopZap {};
	}

	template<>
	class WorkerState<void>
	{
	public:
		WorkerState(UrFelt* papp) : m_papp(papp) {}
		virtual void tick(const float dt) = 0;
	protected:
		UrFelt* m_papp;
	};


	template<>
	class WorkerState<State::InitApp> :	public WorkerState<void>
	{
	public:
		using ThisType = WorkerState<State::InitApp>;
		WorkerState(UrFelt* papp)
		:	WorkerState<void>(papp),
		  	m_co{std::bind(&ThisType::execute, this, std::placeholders::_1)}
		{}
		void tick(const float dt);
	private:
		co::coroutine<FLOAT>::pull_type m_co;
		void execute(co::coroutine<FLOAT>::push_type& sink)
		{
			this->m_papp->m_surface.seed(Vec3i(0,0,0));

			for (UINT i = 0; i < 2; i++)
				this->m_papp->m_surface.update([](auto& pos, auto& phi)->FLOAT {
					return -1.0f;
				});

			const UINT num_children = this->m_papp->m_surface.isogrid().children().data().size();
			for (UINT child_idx = 0; child_idx < num_children; child_idx++)
			{
				this->m_papp->m_surface.init_physics(child_idx);
				sink((FLOAT)child_idx / num_children);
			}
			sink(1.0f);
		}
	};

	template<>
	class WorkerState<State::Running> :	public WorkerState<void>
	{
	public:
		WorkerState(UrFelt* papp) :	WorkerState<void>(papp), m_time_since_update(0) {}
		void tick(const float dt);
	private:
		FLOAT m_time_since_update;
	};


	template<>
	class WorkerState<State::UpdateGPU> :	public WorkerState<void>
	{
	public:
		WorkerState(UrFelt* papp) :	WorkerState<void>(papp) {}
		void tick(const float dt);
	};


	template<>
	class WorkerState<State::Zap> : public WorkerState<void>
	{
	public:
		WorkerState(UrFelt* papp, FLOAT amt) : WorkerState<void>(papp), m_amt(amt) {}
		void tick(const float dt)
		{
			using namespace Urho3D;
			const IntVector2& pos_mouse = m_papp->GetSubsystem<Input>()->GetMousePosition();
			const FLOAT screen_width = m_papp->GetSubsystem<Graphics>()->GetWidth();
			const FLOAT screen_height = m_papp->GetSubsystem<Graphics>()->GetHeight();
			const Vector2 screen_coord{
				FLOAT(pos_mouse.x_) / screen_width,
				FLOAT(pos_mouse.y_) / screen_height
			};

			const Ray& zap_ray = m_papp->GetSubsystem<Renderer>()->GetViewport(0)->GetScene()->
				GetComponent<Camera>("Camera")->GetScreenRay(screen_coord.x_, screen_coord.y_);

			m_papp->m_surface.update_start();
			FLOAT leftover = m_papp->m_surface.delta_gauss<4>(
				reinterpret_cast<const Vec3f&>(zap_ray.origin_),
				reinterpret_cast<const Vec3f&>(zap_ray.direction_),
				m_amt, 2.0f
			);
			m_papp->m_surface.update_end_local();
			m_papp->m_surface.poly().notify(m_papp->m_surface);
		}
	protected:
		const FLOAT m_amt;
	};


	template<>
	class WorkerState<State::InitSurface> : public WorkerState<void>
	{
	public:
		using ThisType = WorkerState<State::InitSurface>;
		WorkerState(UrFelt* papp)
		:	WorkerState<void>(papp),
		  	m_co{std::bind(&ThisType::execute, this, std::placeholders::_1)}
		{}
		void tick(const float dt);
	private:
		co::coroutine<FLOAT>::pull_type m_co;
		void execute(co::coroutine<FLOAT>::push_type& sink)
		{
			const UINT num_children = this->m_papp->m_surface.isogrid().children().data().size();
			for (UINT expand = 0; expand < 100; expand++)
			{
				m_papp->m_surface.update([](auto& pos, auto& phi)->FLOAT {
					using namespace felt;
					if (std::abs(pos(1)) > 1)
						return 0;
					else
						return -1;
				});
				sink(FLOAT(expand)/100);
			}
		}
	};


	struct WorkerRunningSM
	{
		WorkerRunningSM() {}

		auto configure() const noexcept
		{
			using namespace msm;

			state<State::Zap> ZAP;

			return msm::make_transition_table(

				"IDLE"_s(H)	+ event<Event::StartZap>	/
				[](UrFelt* papp, const Event::StartZap& evt) {
					papp->m_worker_state = std::unique_ptr<WorkerState<State::Zap>>(
						new WorkerState<State::Zap>{papp, evt.amt}
					);
				}										=	ZAP,

				ZAP			+ event<Event::StartZap>	/
				[](UrFelt* papp, const Event::StartZap& evt) {
					papp->m_worker_state = std::unique_ptr<WorkerState<State::Zap>>(
						new WorkerState<State::Zap>{papp, evt.amt}
					);
				}										=	ZAP,

				ZAP			+ event<Event::StopZap>		/
				[](UrFelt* papp, const Event::StartZap& evt) {
					papp->m_worker_state.reset(nullptr);
				}										=	"IDLE"_s
			);
		}
	};

	struct AppSM
	{
		template <class EventType>
		static std::function<void (UrFelt*)> trigger(EventType event_);

		static std::function<void (UrFelt*)> initialised();

		static std::function<void (UrFelt*)> app_initialised();
		static std::function<void (UrFelt*)> app_idleing();
		template <class StateTypeApp, class StateTypeWorker>
		static std::function<bool (UrFelt*)> is(
			StateTypeApp state_app_, StateTypeWorker state_worker_
		);
		template <class StateTypeApp, class StateTypeWorker>
		static std::function<bool (UrFelt*)> is_not(
			StateTypeApp state_app_, StateTypeWorker state_worker_
		);
		static std::function<void (UrFelt*)> worker_initialised();
		static std::function<void (UrFelt*)> worker_idleing();

		static std::function<void (UrFelt*)> update_gpu();

		auto configure() const noexcept
		{
			using namespace msm;

			state<State::InitApp> INIT_APP;
			state<sm<WorkerRunningSM>> WORKER_RUNNING;
			state<State::InitSurface> WORKER_INIT;

			return msm::make_transition_table(

*"BOOTSTRAP"_s			+ "load"_t
/ [](UrFelt* papp) {
	papp->m_app_state = std::unique_ptr<WorkerState<void>>(
		new WorkerState<State::InitApp>{papp}
	);
}													= INIT_APP,

INIT_APP				+ "app_initialised"_t		[is(INIT_APP, "WORKER_IDLE"_s)]
/ trigger("initialised"_t)							= "APP_RUNNING"_s,

INIT_APP				+ "app_initialised"_t		[!is(INIT_APP, "WORKER_IDLE"_s)]
/ app_idleing()										= "APP_IDLE"_s,

"APP_IDLE"_s			+ "initialised"_t
/ [](UrFelt* papp) {
	papp->m_app_state = std::unique_ptr<WorkerState<void>>(
		new WorkerState<State::Running>{papp}
	);
}													= "APP_RUNNING"_s,

"APP_RUNNING"_s 		+ "activate_surface"_t
/ [](UrFelt* papp) {
	papp->m_surface_body->Activate();
},

"APP_RUNNING"_s			+ "update_gpu"_t
/ app_idleing()										= "APP_AWAIT_WORKER"_s,

"APP_AWAIT_WORKER"_s								[is("APP_AWAIT_WORKER"_s, "WORKER_PAUSED"_s)]
/ [](UrFelt* papp) {
	papp->m_app_state = std::unique_ptr<WorkerState<void>>(
		new WorkerState<State::UpdateGPU>{papp}
	);
}													= "APP_UPDATE_GPU"_s,

"APP_UPDATE_GPU"_s		+ "resume"_t				= "APP_RUNNING"_s,



*"WORKER_BOOTSTRAP"_s	+ "load"_t
/ [](UrFelt* papp) {
	papp->m_worker_state = std::unique_ptr<WorkerState<State::InitSurface>>(
		new WorkerState<State::InitSurface>{papp}
	);
}													= WORKER_INIT,

WORKER_INIT				+ "worker_initialised"_t	[is("APP_IDLE"_s, WORKER_INIT)]
/ trigger("initialised"_t)							= WORKER_RUNNING,

WORKER_INIT				+ "worker_initialised"_t	[!is("APP_IDLE"_s, WORKER_INIT)]
/ worker_idleing()									= "WORKER_IDLE"_s,

"WORKER_IDLE"_s			+ "initialised"_t			= WORKER_RUNNING,

WORKER_RUNNING 			+ "repoly"_t
/ [](UrFelt* papp) {
	papp->m_surface.poly().surf(papp->m_surface);
},

WORKER_RUNNING			+ "update_gpu"_t			= "WORKER_PAUSING"_s,

"WORKER_PAUSING"_s		+ "worker_paused"_t			= "WORKER_PAUSED"_s,

"WORKER_PAUSED"_s		+ "resume"_t				= WORKER_RUNNING

			);
		}
	};


	class AppController : public msm::sm<AppSM>
	{
	public:
		AppController(UrFelt* app_)
		: m_sm_running_conf(), m_sm_running{m_sm_running_conf},
		  msm::sm<AppSM>{std::move(app_), m_sm_running}
		{}
	private:
//		AppSM m_sm_conf;
		WorkerRunningSM m_sm_running_conf;
		msm::sm<WorkerRunningSM> m_sm_running;
	};
}
#endif /* INCLUDE_URFELT_HPP_ */
