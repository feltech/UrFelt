#ifndef INCLUDE_APPSTATE_HPP_
#define INCLUDE_APPSTATE_HPP_

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


//#define BOOST_MSM_LITE_LOG(T, SM, ...) log_##T<SM>(__VA_ARGS__)

#define BOOST_MSM_LITE_THREAD_SAFE
#include <boost/msm-lite.hpp>

#include <boost/coroutine/all.hpp>
#include "Application.hpp"

namespace msm = boost::msm::lite;
namespace co = boost::coroutines;

#define MAKE_STATE_TYPE(Name) \
	struct Name##Label { static auto c_str() BOOST_MSM_LITE_NOEXCEPT { return #Name; } }; \
	using Name = msm::state<Name##Label>;

namespace UrFelt
{
	namespace State
	{
		namespace Label
		{
			MAKE_STATE_TYPE(Idle)
			MAKE_STATE_TYPE(WorkerIdle)
			MAKE_STATE_TYPE(InitApp)
			MAKE_STATE_TYPE(InitSurface)
			MAKE_STATE_TYPE(Zap)
			MAKE_STATE_TYPE(Running)
			MAKE_STATE_TYPE(UpdateGPU)
			MAKE_STATE_TYPE(UpdatePoly)
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


namespace UrFelt
{
	namespace State
	{
		class TickBase
		{
		public:
			TickBase(Application* papp_) : m_papp(papp_) {}
			virtual void tick(const float dt) = 0;
		protected:
			Application* m_papp;
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
		class Tick<State::Label::Running> :	public TickBase
		{
		public:
			Tick(Application* papp_) :	TickBase(papp_), m_time_since_update(0) {}
			void tick(const float dt);
		private:
			float m_time_since_update;
		};


		template<>
		class Tick<State::Label::UpdateGPU> :	public TickBase
		{
		public:
			Tick(Application* papp_) :	TickBase(papp_) {}
			void tick(const float dt);
		};

		template<>
		class Tick<State::Label::UpdatePoly> :	public TickBase
		{
		public:
			Tick(Application* papp_) :	TickBase(papp_) {}
			void tick(const float dt);
		};


		template<>
		class Tick<State::Label::Zap> : public TickBase
		{
		public:
			Tick(Application* papp_, float amt);
			void tick(const float dt);
		protected:
			const float m_amt;
			const float m_screen_width;
			const float m_screen_height;
			const Urho3D::Camera*	m_pcamera;
		};


		template<>
		class Tick<State::Label::InitSurface> : public TickBase
		{
		public:
			using ThisType = Tick<State::Label::InitSurface>;
			Tick(Application* papp_)
			:	TickBase(papp_),
				m_co{std::bind(&ThisType::execute, this, std::placeholders::_1)}
			{}
			void tick(const float dt);
		private:
			co::coroutine<float>::pull_type m_co;
			void execute(co::coroutine<float>::push_type& sink);
		};

		class WorkerRunningController;

		struct BaseSM
		{
			template <class StateType>
			static std::function<void (Application*)> app_set();

			template <class StateType>
			static std::function<void (Application*)> worker_set();

			std::function<void(WorkerRunningController*, Application*)> worker_restore() const;
		};


		struct WorkerRunningSM : BaseSM
		{
			WorkerRunningSM() {}

			auto configure() noexcept
			{
				using namespace msm;
				using namespace State::Label;

				state<State::Label::Zap> ZAP;

				return msm::make_transition_table(
"WORKER_BOOTSTRAP"_s(H)		+ "load"_t
/ worker_remember<InitSurface>()						= InitSurface{},

InitSurface{}				+ "initialised"_t
/ worker_remember<WorkerIdle>()							= "IDLE"_s,

"IDLE"_s					+ event<Event::StartZap>
/ worker_remember_zap()									=	ZAP,

ZAP							+ event<Event::StopZap>
/ (worker_remember<WorkerIdle>())						=	"IDLE"_s

				);
			}

		private:
			using BaseAction = std::function<void(WorkerRunningController*, Application*)>;
			using ZapAction =
				std::function<void(WorkerRunningController*, Application*, const Event::StartZap&)>;
			ZapAction worker_remember_zap() const;

			template <class StateType>
			BaseAction worker_remember() const;
		};


		class AppSM : BaseSM
		{
		public:
			auto configure() const noexcept
			{
				using namespace msm;
				using namespace State::Label;

				state<sm<WorkerRunningSM>> WORKER_RUNNING;

				return msm::make_transition_table(

*"BOOTSTRAP"_s			+ "load"_t
/ app_set<Running>()								= Running{},

//Running{}		 		+ "activate_surface"_t,
/// [](Application* papp_) {
//papp_->m_psurface_body->Activate();
//},

Running{}				+ "update_gpu"_t
/ app_set<Idle>()									= "APP_AWAIT_WORKER"_s,

"APP_AWAIT_WORKER"_s	+ "worker_pause"_t
/ app_set<UpdateGPU>()								= UpdateGPU{},

UpdateGPU{}				+ "resume"_t
/ app_set<Running>()								= Running{},


*WORKER_RUNNING			+ "update_gpu"_t
/ worker_set<UpdatePoly>()							= UpdatePoly{},

UpdatePoly{}			+ "worker_pause"_t
/ worker_set<WorkerIdle>()							= WorkerIdle{},

WorkerIdle{}			+ "resume"_t				= WORKER_RUNNING,

WORKER_RUNNING			+ msm::on_entry
/ worker_restore()
				);
			}

		private:
			template <class StateType>
			static std::function<bool (Application*)> is(
				StateType state_
			);
		};


		class WorkerRunningController : public msm::sm<WorkerRunningSM>
		{
			friend class AppSM;
		public:
			WorkerRunningController(Application* app_)
			:	msm::sm<WorkerRunningSM>{
					std::move(app_),
					std::move(this),
					m_conf
				}
			{}
			void remember(Application* papp_, TickBase* ticker_)
			{
				m_worker_state.reset(ticker_);
				papp_->m_worker_state_next = m_worker_state;
			}

			void restore(Application* papp_)
			{
				papp_->m_worker_state_next = m_worker_state;
			}
		private:
			std::shared_ptr<TickBase>	m_worker_state;
			WorkerRunningSM 			m_conf;
		};


		class AppController : public msm::sm<AppSM>
		{
		public:
			AppController(Application* papp_)
			:	m_worker_controller(papp_),
				msm::sm<AppSM>{
					std::move(papp_),
					std::move(&m_worker_controller),
					static_cast<msm::sm<WorkerRunningSM>&>(m_worker_controller)
				}
			{}
			static const msm::state<msm::sm<WorkerRunningSM>> WORKER_RUNNING;
		private:
			WorkerRunningController m_worker_controller;
		};



		template <class StateType>
		std::function<bool (UrFelt::Application*)> AppSM::is(
			StateType state_
		) {
			return [state_](UrFelt::Application* papp_) {
				return papp_->m_controller->is(state_);
			};
		}


		template <class StateType>
		std::function<void (UrFelt::Application*)> BaseSM::app_set()
		{
			using namespace msm;

			return [](UrFelt::Application* papp_) {
				papp_->m_app_state_next.reset(new Tick<StateType>{papp_});
			};
		}


		template <class StateType>
		std::function<void (UrFelt::Application*)> BaseSM::worker_set()
		{
			using namespace msm;

			return [](UrFelt::Application* papp_) {
				papp_->m_worker_state_next.reset(new Tick<StateType>{papp_});
			};
		}

		template <class StateType>
		WorkerRunningSM::BaseAction WorkerRunningSM::worker_remember() const
		{
			return [](WorkerRunningController* pworker, Application* papp_) {
				pworker->remember(papp_, new Tick<StateType>{papp_});
			};
		}

	}
}

#endif /* INCLUDE_APPSTATE_HPP_ */
