#include <typeinfo>
#include <stdio.h>

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

#include "UrFelt.hpp"
#include "AppState.hpp"

using namespace felt::State;


void Tick<Label::InitApp>::tick(const float dt)
{
	using namespace Messages;
	if (m_co)
	{
		const FLOAT frac_done = m_co.get();
		this->m_papp->m_queue_script.push(UrQueue::Map{
			{"type", PERCENT_TOP},
			{"value", (FLOAT)(INT)(100.0 * frac_done)},
			{"label", "Initialising physics"}
		});
		m_co();
	}
	else
	{
		m_papp->m_queue_script.push(UrQueue::Map{
			{"type", PERCENT_TOP},
			{"value", -1}
		});
		using namespace msm;
		m_papp->m_controller->process_event("app_initialised"_t);
	}
}


void Tick<Label::InitApp>::execute(co::coroutine<FLOAT>::push_type& sink)
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
}


void Tick<Label::InitSurface>::tick(const float dt)
{
	using namespace Messages;
	if (m_co)
	{
		const FLOAT frac_done = m_co.get();
		m_papp->m_queue_script.push(UrQueue::Map{
			{"type", PERCENT_BOTTOM},
			{"value", (FLOAT)(INT)(100.0 * frac_done)},
			{"label", "Initialising surface"}
		});
		m_co();
	}
	else
	{
		m_papp->m_queue_script.push(UrQueue::Map{
			{"type", PERCENT_BOTTOM},
			{"value", -1}
		});
		using namespace msm;
		m_papp->m_controller->process_event("worker_initialised"_t);
	}
}


void Tick<Label::InitSurface>::execute(co::coroutine<FLOAT>::push_type& sink)
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
}


Tick<Label::Zap>::Tick(UrFelt* papp_, FLOAT amt)
: TickBase(papp_), m_amt(amt),
  m_screen_width(papp_->GetSubsystem<Urho3D::Graphics>()->GetWidth()),
  m_screen_height(papp_->GetSubsystem<Urho3D::Graphics>()->GetHeight()),
  m_pcamera(
	  papp_->GetSubsystem<Urho3D::Renderer>()->GetViewport(0)->GetScene()->
	  	  GetComponent<Urho3D::Camera>("Camera")
  )
{

}


void Tick<Label::Zap>::tick(const float dt)
{
	using namespace Urho3D;
	const IntVector2& pos_mouse = m_papp->GetSubsystem<Input>()->GetMousePosition();
	const Vector2 screen_coord{
		FLOAT(pos_mouse.x_) / m_screen_width,
		FLOAT(pos_mouse.y_) / m_screen_height
	};

	const Ray& zap_ray = m_pcamera->GetScreenRay(screen_coord.x_, screen_coord.y_);

	m_papp->m_surface.update_start();
	FLOAT leftover = m_papp->m_surface.delta_gauss<4>(
		reinterpret_cast<const Vec3f&>(zap_ray.origin_),
		reinterpret_cast<const Vec3f&>(zap_ray.direction_),
		m_amt, 2.0f
	);
	m_papp->m_surface.update_end_local();
	m_papp->m_surface.poly().notify(m_papp->m_surface);
}


void Tick<Label::Running>::tick(const float dt)
{
	m_time_since_update += dt;
	if (m_time_since_update > 1.0f/30.0f)
	{
		using namespace msm;
		m_papp->m_controller->process_event("update_gpu"_t);
	}
}


void Tick<Label::UpdateGPU>::tick(const float dt)
{
	using namespace msm;
	m_papp->m_surface.poly().update_gpu();
	m_papp->m_surface.poly().update_end();
	m_papp->m_controller->process_event("resume"_t);
}


void Tick<Label::UpdatePoly>::tick(const float dt)
{
	using namespace msm;
	m_papp->m_surface.poly().poly_cubes(m_papp->m_surface);
	m_papp->m_controller->process_event("worker_pause"_t);
}

