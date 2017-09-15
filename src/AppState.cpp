#include <Application.hpp>
#include <typeinfo>
#include <stdio.h>
#include <chrono>
#include "Common.hpp"
#include "AppState.hpp"

using namespace UrFelt::State;
using namespace Felt;

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
	}
}


void Tick<Label::InitSurface>::execute(co::coroutine<FLOAT>::push_type& sink)
{
	m_papp->m_psurface->seed(Urho3D::Vector3(0,0,0));

	for (UINT i = 0; i < 2; i++)
		m_papp->m_psurface->update([](const auto&, const auto&)->FLOAT {
			return -1.0f;
		});

	for (UINT expand = 0; expand < 100; expand++)
	{
		m_papp->m_psurface->update([](const auto& pos, const auto&)->FLOAT {
			using namespace Felt;
			if (std::abs(pos(1)) > 1)
				return 0;
			else
				return -1;
		});
		sink(FLOAT(expand)/100);
	}
}


Tick<Label::Zap>::Tick(Application* papp_, FLOAT amt)
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
	const Vec3f& origin = reinterpret_cast<const Vec3f&>(zap_ray.origin_);
	const Vec3f& direction = reinterpret_cast<const Vec3f&>(zap_ray.direction_);

	constexpr float radius = 5.0f;

	const Vec3f& pos_hit = m_papp->m_psurface->ray(origin, direction);

	if (pos_hit == UrSurface::ray_miss)
		return;

	const Vec3i& pos_lower = pos_hit.array().floor().matrix().template cast<INT>() -
		Vec3i::Constant(UINT(radius));
	const Vec3i& pos_upper = pos_hit.array().ceil().matrix().template cast<INT>() +
		Vec3i::Constant(UINT(radius));

	using Clock = std::chrono::high_resolution_clock;
	using Seconds = std::chrono::duration<float>;
	using Time = std::chrono::time_point<Clock, Seconds>;
	using namespace Messages;

	auto time_before = Clock::now();

	m_papp->m_psurface->update(pos_lower, pos_upper,
		[&pos_hit, this](const Vec3i& pos, const UrSurface::IsoGrid& isogrid) -> FLOAT {
			const Vec3f& posf = pos.template cast<FLOAT>();
			const Vec3f& pos_dist = posf - pos_hit;
			const FLOAT dist = pos_dist.norm() - radius;
			const FLOAT speed = this->m_amt * std::min(dist / radius, 0.0f);
//			const FLOAT curv = isogrid.curv(pos);
			if (speed != speed)
			{
				std::stringstream strs;
				strs << "Invalid speed: " << speed;
				std::string str = strs.str();
				throw std::domain_error(str);
			}
			if (!(-1 < speed && speed < 1))
			{
				std::stringstream strs;
				strs << "Invalid speed: " << speed;
				std::string str = strs.str();
				throw std::domain_error(str);
			}
			return 0.3f*speed;
		}
	);


	auto time_after = Clock::now();
	Seconds duration = time_after - time_before;

	m_papp->m_queue_script.push(UrQueue::Map{
		{"type", PERCENT_TOP},
		{"value", duration.count() * 1000},
		{"label", "Updating surface"}
	});


//	m_papp->m_psurface->update_start();
//	FLOAT leftover = m_papp->m_psurface->delta_gauss<4>(
//		reinterpret_cast<const Vec3f&>(zap_ray.origin_),
//		reinterpret_cast<const Vec3f&>(zap_ray.direction_),
//		m_amt, radius
//	);
//	m_papp->m_psurface->update_end_local();
//	m_papp->m_psurface->poly().notify(m_papp->m_psurface);
}


void Tick<Label::Running>::tick(const float dt)
{
	m_time_since_update += dt;
	if (m_time_since_update > 1.0f/30.0f)
	{
	}
}


void Tick<Label::UpdateGPU>::tick(const float dt)
{
	m_papp->m_psurface->flush();
}


void Tick<Label::UpdatePoly>::tick(const float dt)
{
	m_papp->m_psurface->polygonise();
}

