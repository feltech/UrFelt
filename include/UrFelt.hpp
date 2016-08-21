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
#include <Urho3D/IO/Log.h>

#include <Felt/Surface.hpp>
#include <LuaCppMsg.hpp>

#include "UrSurface3D.hpp"

extern int tolua_UrFelt_open (lua_State* tolua_S);


template <class SM, class TEvent>
void log_process_event(const TEvent& evt) {
	printf("[%s][process_event] %s\n", typeid(SM).name(), evt.c_str());
}

template <class SM, class TGuard, class TEvent>
void log_guard(const TGuard&, const TEvent& evt, bool result) {
	printf("[%s][guard] %s %s %s\n", typeid(SM).name(), typeid(TGuard).name(), evt.c_str(),
		(result ? "[OK]" : "[Reject]"));
}

template <class SM, class TAction, class TEvent>
void log_action(const TAction&, const TEvent& evt) {
	printf("[%s][action] %s %s\n", typeid(SM).name(), typeid(TAction).name(), evt.c_str());
}

template <class SM, class TSrcState, class TDstState>
void log_state_change(const TSrcState& src, const TDstState& dst) {
	printf("[%s][transition] %s -> %s\n", typeid(SM).name(), src.c_str(), dst.c_str());
}

#define BOOST_MSM_LITE_LOG(T, SM, ...) log_##T<SM>(__VA_ARGS__)
#define BOOST_MSM_LITE_THREAD_SAFE
#include <boost/msm-lite.hpp>

#define STATE_STR(StateClass) \
namespace boost { namespace msm { namespace lite { namespace v_1_0_1 { namespace detail \
{ \
template <> \
struct state_str<boost::msm::lite::state<felt::State::StateClass>> { \
  static auto c_str() BOOST_MSM_LITE_NOEXCEPT { return #StateClass; } \
}; \
}}}}}

namespace felt
{
namespace State
{
	struct Idle{};
	struct WorkerIdle{};
	struct InitApp{};
	struct InitSurface{};
	struct Zap{};
	struct Running{};
	struct UpdateGPU{};
	struct UpdatePoly{};
}
}

STATE_STR(Idle)
STATE_STR(WorkerIdle)
STATE_STR(InitApp)
STATE_STR(InitSurface)
STATE_STR(Zap)
STATE_STR(Running)
STATE_STR(UpdateGPU)
STATE_STR(UpdatePoly)


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
		friend struct BaseSM;
		friend struct AppSM;
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
		std::unique_ptr<WorkerState<State::Idle>>	m_app_state;
		std::shared_ptr<WorkerState<State::Idle>>	m_worker_state;
		std::unique_ptr<WorkerState<State::Idle>>	m_app_state_next;
		std::shared_ptr<WorkerState<State::Idle>>	m_worker_state_next;

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


	namespace Event
	{
		struct StartZap
		{
			const float amt;
			static auto c_str() BOOST_MSM_LITE_NOEXCEPT {
				return "StartZap";
			}
		};
		struct StopZap
		{
			static auto c_str() BOOST_MSM_LITE_NOEXCEPT {
				return "StopZap";
			}
		};
	}


	template<>
	class WorkerState<State::Idle>
	{
	public:
		WorkerState(UrFelt* papp) : m_papp(papp) {}
		virtual void tick(const float dt) {};
	protected:
		UrFelt* m_papp;
	};

	template<> class WorkerState<State::WorkerIdle> : public WorkerState<State::Idle>
	{
		using WorkerState<State::Idle>::WorkerState;
	};

	template<>
	class WorkerState<State::InitApp> :	public WorkerState<State::Idle>
	{
	public:
		using ThisType = WorkerState<State::InitApp>;
		WorkerState(UrFelt* papp)
		:	WorkerState<State::Idle>(papp),
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
	class WorkerState<State::Running> :	public WorkerState<State::Idle>
	{
	public:
		WorkerState(UrFelt* papp) :	WorkerState<State::Idle>(papp), m_time_since_update(0) {}
		void tick(const float dt);
	private:
		FLOAT m_time_since_update;
	};


	template<>
	class WorkerState<State::UpdateGPU> :	public WorkerState<State::Idle>
	{
	public:
		WorkerState(UrFelt* papp) :	WorkerState<State::Idle>(papp) {}
		void tick(const float dt);
	};

	template<>
	class WorkerState<State::UpdatePoly> :	public WorkerState<State::Idle>
	{
	public:
		WorkerState(UrFelt* papp) :	WorkerState<State::Idle>(papp) {}
		void tick(const float dt);
	};

	template<>
	class WorkerState<State::Zap> : public WorkerState<State::Idle>
	{
	public:
		WorkerState(UrFelt* papp, FLOAT amt) : WorkerState<State::Idle>(papp), m_amt(amt) {}
		void tick(const float dt);
	protected:
		const FLOAT m_amt;
	};


	template<>
	class WorkerState<State::InitSurface> : public WorkerState<State::Idle>
	{
	public:
		using ThisType = WorkerState<State::InitSurface>;
		WorkerState(UrFelt* papp)
		:	WorkerState<State::Idle>(papp),
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
			m_papp->m_surface.poly().surf(m_papp->m_surface);
			sink(1.0f);
		}
	};

	struct BaseSM
	{
		template <class StateType>
		static std::function<void (UrFelt*)> app_set();

		template <class StateType>
		static std::function<void (UrFelt*)> worker_set();
	};

	struct WorkerRunningSM : BaseSM
	{
		WorkerRunningSM() {}

		auto remember()
		{
			return [this](UrFelt* papp, std::shared_ptr<WorkerState<State::Idle>>* worker_state) {
				volatile int i = 0;
				*worker_state = papp->m_worker_state_next;
			};
		}

		auto forget()
		{
			return [](std::shared_ptr<WorkerState<State::Idle>>* worker_state) {
				volatile int i = 0;
				worker_state->reset();
			};
		}

		auto restore()
		{
			return [](UrFelt* papp, std::shared_ptr<WorkerState<State::Idle>>* worker_state) {
				volatile int i = 0;
				if (*worker_state)
					papp->m_worker_state_next = *worker_state;
			};
		}

		auto has_state()
		{
			return [](std::shared_ptr<WorkerState<State::Idle>>* worker_state) {
				return !!*worker_state;
			};
		}

		auto tmp()
		{
			return [this](UrFelt* papp) {
				volatile int i = 0;
			};
		}
		auto configure() noexcept
		{
			using namespace msm;

			state<State::Zap> ZAP;

			return msm::make_transition_table(

"IDLE"_s(H)	+ event<Event::StartZap>
/ [](UrFelt* papp, const Event::StartZap& evt) {
	papp->m_worker_state_next = std::unique_ptr<WorkerState<State::Idle>>(
		new WorkerState<State::Zap>{papp, evt.amt}
	);
}												=	ZAP,

ZAP			+ event<Event::StopZap>
/ (worker_set<State::WorkerIdle>(), forget())	=	"IDLE"_s,

ZAP			+ msm::on_entry 					[has_state()]
/ restore(),
ZAP			+ msm::on_entry 					[!has_state()]
/ remember()

			);
		}
	private:
		std::shared_ptr<WorkerState<State::Idle>>	m_worker_state;
	};

	struct AppSM : BaseSM
	{
		template <class StateTypeApp, class StateTypeWorker>
		static std::function<bool (UrFelt*)> is(
			StateTypeApp state_app_, StateTypeWorker state_worker_
		);
		static std::function<void (UrFelt*)> app_idleing();
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
/ app_set<State::InitApp>()							= INIT_APP,

INIT_APP				+ "app_initialised"_t		[is(INIT_APP, "WORKER_IDLE"_s)]
/ (process_event("initialised"_t), app_set<State::Running>())
													= "APP_RUNNING"_s,

INIT_APP				+ "app_initialised"_t		[!is(INIT_APP, "WORKER_IDLE"_s)]
/ app_set<State::Idle>()							= "APP_IDLE"_s,

"APP_IDLE"_s			+ "initialised"_t
/ app_set<State::Running>()							= "APP_RUNNING"_s,

"APP_RUNNING"_s 		+ "activate_surface"_t
/ [](UrFelt* papp) {
	papp->m_surface_body->Activate();
},

"APP_RUNNING"_s			+ "update_gpu"_t
/ app_set<State::Idle>()							= "APP_AWAIT_WORKER"_s,

"APP_AWAIT_WORKER"_s	+ "worker_pause"_t
/ app_set<State::UpdateGPU>()						= "APP_UPDATE_GPU"_s,

"APP_UPDATE_GPU"_s		+ "resume"_t
/ app_set<State::Running>()							= "APP_RUNNING"_s,



*"WORKER_BOOTSTRAP"_s	+ "load"_t
/ worker_set<State::InitSurface>()					= WORKER_INIT,

WORKER_INIT				+ "worker_initialised"_t	[is("APP_IDLE"_s, WORKER_INIT)]
/ (process_event("initialised"_t), worker_set<State::WorkerIdle>())
													= WORKER_RUNNING,

WORKER_INIT				+ "worker_initialised"_t	[!is("APP_IDLE"_s, WORKER_INIT)]
/ worker_set<State::WorkerIdle>()					= "WORKER_IDLE"_s,

"WORKER_IDLE"_s			+ "initialised"_t			= WORKER_RUNNING,

WORKER_RUNNING			+ "update_gpu"_t
/ worker_set<State::UpdatePoly>()					= "WORKER_UPDATE_POLY"_s,

"WORKER_UPDATE_POLY"_s	+ "worker_pause"_t
/ worker_set<State::WorkerIdle>()					= "WORKER_PAUSED"_s,

"WORKER_PAUSED"_s		+ "resume"_t				= WORKER_RUNNING,

WORKER_RUNNING			+ msm::on_entry
/ []{} // Workaround for https://github.com/boost-experimental/msm-lite/issues/53

			);
		}
	};


	class WorkerRunningController : public msm::sm<WorkerRunningSM>
	{
	public:
		WorkerRunningController(UrFelt* app_)
		:	msm::sm<WorkerRunningSM>(
				std::move(app_), &m_worker_state, conf
			)
		{}
	private:
		WorkerRunningSM conf;
		std::shared_ptr<WorkerState<State::Idle>> m_worker_state;
	};

	class AppController : public msm::sm<AppSM>
	{
	public:
		AppController(UrFelt* app_)
		:
			m_sm_running(app_),
			msm::sm<AppSM>{
				std::move(app_),
				static_cast<msm::sm<WorkerRunningSM>&>(m_sm_running)
			}
		{}

		static const msm::state<msm::sm<WorkerRunningSM>> WORKER_RUNNING;
	private:
		WorkerRunningController m_sm_running;
	};
}

#endif /* INCLUDE_URFELT_HPP_ */
