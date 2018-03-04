#include "Op/TransformToSphere.hpp"

namespace UrFelt
{
namespace Op
{
namespace TransformToSphere
{


Impl::Impl(const Urho3D::Vector3& pos_centre_, const float radius_, sol::function callback_) :
	Base{callback_},
	m_is_complete{false},
	m_radius{radius_},
	m_pos_centre{reinterpret_cast<const Felt::Vec3f&>(pos_centre_)},
	m_size{0}
{}


template <int TDir>
void Impl::execute(UrSurface& surface_, const Felt::Vec3i& pos_min_, const Felt::Vec3i& pos_max_)
{
	// Assume complete until at least one point has a distance update greater than epsilon.
	m_is_complete = true;
	if (this->m_cancelled)
		return;

	surface_.update(
		pos_min_, pos_max_,
		[this](const Felt::Vec3i& pos_, const IsoGrid& isogrid_) {
			using namespace Felt;
			// Size of surface update to consider zero (thus finished).
			static constexpr Distance epsilon = 0.0001;
			// Multiplier to ensure no surface update with magnitude greater than 0.5.
			static constexpr Distance clamp = 0.5f;

			// Get entropy-satisfying gradient (surface "normal").
			const Vec3f& grad = isogrid_.gradE(pos_);
			Distance dist;
			// If the gradient is zero, we must have a singularity.
			if (grad.isZero())
			{
				const Vec3f& posf = pos_.template cast<Felt::Distance>();
				const Vec3f& pos_dist = m_pos_centre - posf;
				dist = pos_dist.norm() - m_radius;
			}
			else
			{
				// Get distance of this discrete zero-layer point to the continuous zero-level set
				// surface.
				const Felt::Distance dist_surf = isogrid_.get(pos_);
				// Discretisation means grad can be non-normalised, so normalise it.
				const Vec3f& normal = grad.normalized();
				// Interpolate discrete zero-layer grid point to continuous zero-level isosurface.
				const Vec3f& posf = pos_.template cast<Felt::Distance>() - normal*dist_surf;
				// Get euclidean norm of the gradient, for use in level set update equations.
				const Distance grad_norm = grad.norm();

				const Vec3f& pos_dist = m_pos_centre - posf;
				dist = grad_norm * (pos_dist.norm() - m_radius);
			}

			// The level set update.
			const Distance impulse = std::max(std::min(dist / m_radius, 0.0f), -1.0f);

			const Distance speed = TDir * clamp * impulse;

			// Flag that the operation is incomplete if this surface point wants to move a distance
			// larger than epsilon.
			m_is_complete &= std::abs(speed) <= epsilon;

			return speed;
		}
	);
}


bool Impl::is_complete()
{
	return m_is_complete;
}


Attract::Attract(
	const Urho3D::Vector3& pos_centre_, const float radius_
) :
	Attract{pos_centre_, radius_, sol::function{}}
{}


Attract::Attract(
	const Urho3D::Vector3& pos_centre_, const float radius_,
	sol::function callback_
) :
	Impl{pos_centre_, radius_, callback_},
	Bounded{
		Felt::Vec3f::Constant(-radius_) + reinterpret_cast<const Felt::Vec3f&>(pos_centre_),
		Felt::Vec3f::Constant(radius_) + reinterpret_cast<const Felt::Vec3f&>(pos_centre_)
	}
{}


void Attract::execute(UrSurface& surface)
{
	Impl::execute<1>(surface, m_pos_min, m_pos_max);
}


Repel::Repel(
	const Urho3D::Vector3& pos_centre_, const float radius_
) :
	Repel{pos_centre_, radius_, sol::function{}}
{}


Repel::Repel(
	const Urho3D::Vector3& pos_centre_, const float radius_,
	sol::function callback_
) :
	Impl{pos_centre_, radius_, callback_},
	Bounded{
		Felt::Vec3f::Constant(-radius_) + reinterpret_cast<const Felt::Vec3f&>(pos_centre_),
		Felt::Vec3f::Constant(radius_) + reinterpret_cast<const Felt::Vec3f&>(pos_centre_)
	}
{}


void Repel::execute(UrSurface& surface)
{
	Impl::execute<-1>(surface, m_pos_min, m_pos_max);
}

} // ExpandToSphere.
} // Op.
} // UrFelt.



