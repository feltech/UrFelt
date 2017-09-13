#ifndef INCLUDE_APPSTATE_HPP_
#define INCLUDE_APPSTATE_HPP_

#include <boost/coroutine/all.hpp>
#include "Application.hpp"

namespace co = boost::coroutines;

namespace UrFelt
{
	namespace State
	{

		namespace Event
		{
			struct StartZap{};
			struct StopZap{};
		}
		namespace Label
		{
			struct Idle{};
			struct WorkerIdle{};
			struct Running{};
			struct UpdateGPU{};
			struct UpdatePoly{};
			struct InitSurface{};
			struct Zap{};
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
	}
}

#endif /* INCLUDE_APPSTATE_HPP_ */
