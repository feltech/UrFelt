#include <Op/Base.hpp>

namespace UrFelt
{
namespace Op
{


Base::Base(sol::function callback_) :
	callback{callback_}, m_cancelled{false}
{}


bool Base::is_complete()
{
	return true;
}


void Base::stop()
{
	m_cancelled = true;
}


Bounded::Bounded(const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_) :
	m_pos_min{
		reinterpret_cast<const Felt::Vec3f&>(pos_min_)
			.array().floor().matrix().template cast<int>()
	},
	m_pos_max{
		reinterpret_cast<const Felt::Vec3f&>(pos_max_)
			.array().ceil().matrix().template cast<int>()
	}
{}


} // Base.
} // UrFelt.
