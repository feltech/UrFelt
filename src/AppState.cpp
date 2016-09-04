#include <typeinfo>
#include <stdio.h>
#include "UrFelt.hpp"
#include "AppState.hpp"

using namespace felt::State;


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
		m_papp->m_controller->process_event("initialised"_t);
	}
}


void Tick<Label::InitSurface>::execute(co::coroutine<FLOAT>::push_type& sink)
{
	m_papp->m_surface.seed(Vec3i(0,0,0));

	for (UINT i = 0; i < 2; i++)
		m_papp->m_surface.update([](auto& pos, auto& phi)->FLOAT {
			return -1.0f;
		});

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
	m_papp->m_surface.flush();
	m_papp->m_controller->process_event("resume"_t);
}


void Tick<Label::UpdatePoly>::tick(const float dt)
{
	using namespace msm;
	m_papp->m_surface.poly().poly_cubes(m_papp->m_surface);
	m_papp->m_controller->process_event("worker_pause"_t);
}
const msm::state<msm::sm<WorkerRunningSM>> AppController::WORKER_RUNNING =
	msm::state<msm::sm<WorkerRunningSM>>{};


std::function<void(WorkerRunningController*, felt::UrFelt*)> BaseSM::worker_restore() const
{
	return [](WorkerRunningController* pworker, UrFelt* papp_) {
		pworker->restore(papp_);
	};
}
WorkerRunningSM::ZapAction WorkerRunningSM::worker_remember_zap() const
{
	return [](WorkerRunningController* pworker, UrFelt* papp_, const Event::StartZap& evt) {
		pworker->remember(papp_, new Tick<State::Label::Zap>{papp_, evt.amt});
	};
}
