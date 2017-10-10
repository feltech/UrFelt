#include <Op/ExpandToSphere.hpp>

namespace UrFelt
{
namespace Op
{
namespace ExpandToSphere
{


Impl::Impl(const Urho3D::Vector3& pos_centre_, const float radius_, sol::function callback_) :
	Base{callback_},
	m_pos_centre{reinterpret_cast<const Felt::Vec3f&>(pos_centre_)},
	m_radius{radius_},
	m_is_complete{false},
	m_size{0}
{}


template <typename... Bounds>
void Impl::execute(UrSurface& surface_, Bounds... bounds_)
{
	// Assume complete until at least one point has a distance update greater than epsilon.
	m_is_complete = true;
	if (this->m_cancelled)
		return;
	// Take a copy of surface size and centre of mass, before resetting it.
	const Felt::Distance size = m_size;
	Felt::Vec3f pos_COM = m_pos_COM;
	// Reset surface size (number of zero layer points) and surface's centre of mass to zero,
	// for incremental recalculation below.
	m_size = 0;
	m_pos_COM = Felt::Vec3f{0,0,0};

	surface_.update(bounds...,
		[this, &pos_COM, &size](const Felt::Vec3i& pos_, const IsoGrid& isogrid_) {
			using namespace Felt;
			// Size of surface update to consider zero (thus finished).
			static constexpr Distance epsilon = 0.0001;
			// Multiplier to ensure no surface update with magnitude greater than 1.0.
			// TODO: understand why grad can be > 2 and if sqrt(2) per component theory is correct.
			static constexpr Distance clamp = 1.0f/sqrt(6.0f);

			// Get entropy-satisfying gradient (surface "normal").
			const Vec3f& grad = isogrid_.gradE(pos_);
			// If the gradient is zero, we must have a singularity, so just trivially contract
			// (i.e. destroy).
			if (grad.isZero())
				return 1.0f;
			// Get distance of this discrete zero-layer point to the continuous zero-level set
			// surface.
			const Felt::Distance dist_surf = isogrid_.get(pos_);

			// Discretisation means grad can be non-normalised, so normalise it.
			const Vec3f& normal = grad.normalized();
			// Interpolate discrete zero-layer grid point to continuous zero-level isosurface.
			const Vec3f& posf = pos_.template cast<Felt::Distance>() - normal*dist_surf;
			// Get euclidean norm of the gradient, for use in level set update equations.
			const Distance grad_norm = grad.norm();

			// Incremental update of centre of mass.
			m_size++;
			m_pos_COM += posf;

			// Direction from centre of mass of surface to this surface point.
			Vec3f dir_from_COM;

			if (size == 0)
			{
				// If size (thus centre of mass) has not yet been calculated (i.e. this is the
				// first iteration) just assume the line from the centre of mass is the same as the
				// surface normal.
				dir_from_COM = normal;
			}
			else
			{
				// Displacement of surface point from surface's centre of mass.
				const Vec3f& disp_from_COM = posf - pos_COM;
				// If the surface is tiny, don't allow it to collapse.
				if (disp_from_COM.squaredNorm() <= 1.0f/clamp)
				{
					m_is_complete = false;
					return -grad_norm*clamp;
				}
				// Get direction of surface point from surface's centre of mass.
				dir_from_COM = disp_from_COM.normalized();
			}

			// Calculate point on sphere where this surface point wants to head toward.
			const Vec3f& pos_target = m_pos_centre + dir_from_COM*m_radius;

			// Get displacement of surface point to it's target point on the sphere.
			const Vec3f& displacement_to_target =  pos_target - posf;

			// Calculate "impulse" to apply to surface point along it's normal to get it heading
			// toward target box point.
			Distance impulse;
			if (displacement_to_target.squaredNorm() > 1.0f)
				// If we're far from the target point, clamp the impulse to +/-1.
				impulse = normal.dot(displacement_to_target.normalized());
			else
				// If we're near the target point, the impulse is directly proportional to the
				// distance.
				impulse = normal.dot(displacement_to_target);

			// The level set update.
			const Felt::Distance amount = -clamp*grad_norm*impulse;

			// Flag that the operation is incomplete if this surface point wants to move a distance
			// larger than epsilon.
			m_is_complete &= std::abs(amount) <= epsilon;

			return amount;
		}
	);

	// Update centre of mass as sum (see above) divided by number of surface points.
	m_pos_COM /= m_size;
}


bool Impl::is_complete()
{
	return m_is_complete;
}


Local::Local(
	const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
	const Urho3D::Vector3& pos_centre_, const float radius_,
) :
	Local{pos_min_, pos_max_, pos_centre_, radius_, sol::function{}}
{}


Local::Local(
	const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
	const Urho3D::Vector3& pos_centre_, const float radius_,
	sol::function callback_
) :
	Impl{pos_centre_, radius_, callback_}, Bounded{pos_min_, pos_max_}
{}


void Local::execute(UrSurface& surface)
{
	Impl::execute(surface, m_pos_min, m_pos_max);
}


Global::Global(const Urho3D::Vector3& pos_centre_, const float radius_) :
	Global{pos_centre_, radius_, sol::function{}}
{}


Global::Global(const Urho3D::Vector3& pos_centre_, const float radius_, sol::function callback_) :
	Impl{pos_centre_, radius_, callback_}
{}


void Global::execute(UrSurface& surface)
{
	Impl::execute(surface);
}

} // ExpandToSphere.
} // Op.
} // UrFelt.



