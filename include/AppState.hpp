#ifndef INCLUDE_APPSTATE_HPP_
#define INCLUDE_APPSTATE_HPP_

#define BOOST_MSM_LITE_THREAD_SAFE
#include <boost/msm-lite.hpp>

#include <boost/coroutine/all.hpp>
#include "UrFelt.hpp"

#define STATE_STR(StateClass) \
namespace boost { namespace msm { namespace lite { namespace v_1_0_1 { namespace detail \
{ \
template <> \
struct state_str<boost::msm::lite::state<felt::State::Label::StateClass>> { \
  static auto c_str() BOOST_MSM_LITE_NOEXCEPT { return #StateClass; } \
}; \
}}}}}


namespace msm = boost::msm::lite;
namespace co = boost::coroutines;


namespace felt
{
	namespace State
	{
		namespace Label
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
	namespace State
	{
		class TickBase
		{
		public:
			TickBase(UrFelt* papp_) : m_papp(papp_) {}
			virtual void tick(const float dt) = 0;
		protected:
			UrFelt* m_papp;
		};


		template<>
		class Tick<State::Label::Idle> : public TickBase
		{
		public:
			using TickBase::TickBase;
			virtual void tick(const float dt) {};
		};


		template<> class Tick<State::Label::WorkerIdle> : public TickBase
		{
			using TickBase::TickBase;
			virtual void tick(const float dt) {};
		};

		template<>
		class Tick<State::Label::InitApp> :	public TickBase
		{
		public:
			using ThisType = Tick<State::Label::InitApp>;
			Tick(UrFelt* papp_)
			:	TickBase(papp_),
				m_co{std::bind(&ThisType::execute, this, std::placeholders::_1)}
			{}
			void tick(const float dt);
		private:
			co::coroutine<FLOAT>::pull_type m_co;
			void execute(co::coroutine<FLOAT>::push_type& sink);
		};


		template<>
		class Tick<State::Label::Running> :	public TickBase
		{
		public:
			Tick(UrFelt* papp_) :	TickBase(papp_), m_time_since_update(0) {}
			void tick(const float dt);
		private:
			FLOAT m_time_since_update;
		};


		template<>
		class Tick<State::Label::UpdateGPU> :	public TickBase
		{
		public:
			Tick(UrFelt* papp_) :	TickBase(papp_) {}
			void tick(const float dt);
		};

		template<>
		class Tick<State::Label::UpdatePoly> :	public TickBase
		{
		public:
			Tick(UrFelt* papp_) :	TickBase(papp_) {}
			void tick(const float dt);
		};


		template<>
		class Tick<State::Label::Zap> : public TickBase
		{
		public:
			Tick(UrFelt* papp_, FLOAT amt) : TickBase(papp_), m_amt(amt) {}
			void tick(const float dt);
		protected:
			const FLOAT m_amt;
		};


		template<>
		class Tick<State::Label::InitSurface> : public TickBase
		{
		public:
			using ThisType = Tick<State::Label::InitSurface>;
			Tick(UrFelt* papp_)
			:	TickBase(papp_),
				m_co{std::bind(&ThisType::execute, this, std::placeholders::_1)}
			{}
			void tick(const float dt);
		private:
			co::coroutine<FLOAT>::pull_type m_co;
			void execute(co::coroutine<FLOAT>::push_type& sink);
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
				return [this](UrFelt* papp_) {
					m_worker_state = papp_->m_worker_state_next;
				};
			}

			auto forget()
			{
				return [this]() {
					m_worker_state.reset();
				};
			}

			auto restore()
			{
				return [this](UrFelt* papp_) {
					if (m_worker_state)
						papp_->m_worker_state_next = m_worker_state;
				};
			}

			auto has_state()
			{
				return [this]() {
					return !!m_worker_state;
				};
			}

			auto configure() noexcept
			{
				using namespace msm;

				state<State::Label::Zap> ZAP;

				return msm::make_transition_table(

"IDLE"_s(H)	+ event<Event::StartZap>
/ [](UrFelt* papp_, const Event::StartZap& evt) {
	papp_->m_worker_state_next.reset(
		new Tick<Label::Zap>{papp_, evt.amt}
	);
}												=	ZAP,

ZAP			+ event<Event::StopZap>
/ (worker_set<Label::WorkerIdle>(), forget())	=	"IDLE"_s,

ZAP			+ msm::on_entry 					[has_state()]
/ restore(),
ZAP			+ msm::on_entry 					[!has_state()]
/ remember()

				);
			}
		private:
			std::shared_ptr<TickBase>	m_worker_state;
		};


		class AppSM : BaseSM
		{
		public:
			auto configure() const noexcept
			{
				using namespace msm;
				using namespace State::Label;

				state<State::Label::InitApp> INIT_APP;
				state<sm<WorkerRunningSM>> WORKER_RUNNING;
				state<State::Label::InitSurface> WORKER_INIT;

				return msm::make_transition_table(

*"BOOTSTRAP"_s			+ "load"_t
/ app_set<InitApp>()								= INIT_APP,

INIT_APP				+ "app_initialised"_t		[is(INIT_APP, "WORKER_IDLE"_s)]
/ (process_event("initialised"_t), app_set<Running>())
													= "APP_RUNNING"_s,

INIT_APP				+ "app_initialised"_t		[!is(INIT_APP, "WORKER_IDLE"_s)]
/ app_set<Idle>()									= "APP_IDLE"_s,

"APP_IDLE"_s			+ "initialised"_t
/ app_set<Running>()								= "APP_RUNNING"_s,

"APP_RUNNING"_s 		+ "activate_surface"_t
/ [](UrFelt* papp_) {
	papp_->m_surface_body->Activate();
},

"APP_RUNNING"_s			+ "update_gpu"_t
/ app_set<Idle>()									= "APP_AWAIT_WORKER"_s,

"APP_AWAIT_WORKER"_s	+ "worker_pause"_t
/ app_set<UpdateGPU>()								= "APP_UPDATE_GPU"_s,

"APP_UPDATE_GPU"_s		+ "resume"_t
/ app_set<Running>()								= "APP_RUNNING"_s,



*"WORKER_BOOTSTRAP"_s	+ "load"_t
/ worker_set<InitSurface>()							= WORKER_INIT,

WORKER_INIT				+ "worker_initialised"_t	[is("APP_IDLE"_s, WORKER_INIT)]
/ (process_event("initialised"_t), worker_set<WorkerIdle>())
													= WORKER_RUNNING,

WORKER_INIT				+ "worker_initialised"_t	[!is("APP_IDLE"_s, WORKER_INIT)]
/ worker_set<WorkerIdle>()							= "WORKER_IDLE"_s,

"WORKER_IDLE"_s			+ "initialised"_t			= WORKER_RUNNING,

WORKER_RUNNING			+ "update_gpu"_t
/ worker_set<UpdatePoly>()							= "WORKER_UPDATE_POLY"_s,

"WORKER_UPDATE_POLY"_s	+ "worker_pause"_t
/ worker_set<WorkerIdle>()							= "WORKER_PAUSED"_s,

"WORKER_PAUSED"_s		+ "resume"_t				= WORKER_RUNNING,

WORKER_RUNNING			+ msm::on_entry
/ []{} // Workaround for https://github.com/boost-experimental/msm-lite/issues/53

				);
			}

		private:
			template <class StateTypeApp, class StateTypeWorker>
			static std::function<bool (UrFelt*)> is(
				StateTypeApp state_app_, StateTypeWorker state_worker_
			);
			static std::function<void (UrFelt*)> app_idleing();
			static std::function<void (UrFelt*)> worker_idleing();

			static std::function<void (UrFelt*)> update_gpu();
		};


		class WorkerRunningController : public msm::sm<WorkerRunningSM>
		{
		public:
			WorkerRunningController(UrFelt* app_)
			:	msm::sm<WorkerRunningSM>(
					std::move(app_), conf
				)
			{}
		private:
			WorkerRunningSM conf;
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



		template <class StateTypeApp, class StateTypeWorker>
		std::function<bool (felt::UrFelt*)> AppSM::is(
			StateTypeApp state_app_, StateTypeWorker state_worker_
		) {
			return [state_app_, state_worker_](felt::UrFelt* papp_) {
				return papp_->m_controller->is(state_app_, state_worker_);
			};
		}


		template <class StateType>
		std::function<void (felt::UrFelt*)> BaseSM::app_set()
		{
			using namespace msm;

			return [](felt::UrFelt* papp_) {
				papp_->m_app_state_next.reset(new Tick<StateType>{papp_});
			};
		}


		template <class StateType>
		std::function<void (felt::UrFelt*)> BaseSM::worker_set()
		{
			using namespace msm;

			return [](felt::UrFelt* papp_) {
				papp_->m_worker_state_next.reset(new Tick<StateType>{papp_});
			};
		}
	}
}

#endif /* INCLUDE_APPSTATE_HPP_ */
