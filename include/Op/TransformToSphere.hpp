#ifndef INCLUDE_OP_ExpandToSphere_HPP_
#define INCLUDE_OP_ExpandToSphere_HPP_

#include <Op/Base.hpp>
#include <UrSurface.hpp>

namespace UrFelt
{
namespace Op
{
namespace TransformToSphere
{


struct Impl : Base
{
	template <int TDir>
	void execute(UrSurface& surface_, const Felt::Vec3i& pos_min_, const Felt::Vec3i& pos_max_);

	bool is_complete();
protected:
	Impl() = delete;
	Impl(const Urho3D::Vector3& pos_centre_, const float radius_, sol::function callback_);

private:
	bool	m_is_complete;
	const float	m_radius;
	const Felt::Vec3f	m_pos_centre;
	Felt::Vec3f	m_pos_COM;
	Felt::ListIdx	m_size;
};


struct Attract : Impl, Bounded
{
	Attract(
		const Urho3D::Vector3& pos_centre_, const float radius_
	);
	Attract(
		const Urho3D::Vector3& pos_centre_, const float radius_,
		sol::function callback_
	);
	void execute(UrSurface& surface);
};


struct Repel : Impl, Bounded
{
	Repel(
		const Urho3D::Vector3& pos_centre_, const float radius_
	);
	Repel(
		const Urho3D::Vector3& pos_centre_, const float radius_,
		sol::function callback_
	);
	void execute(UrSurface& surface);
};


} // ExpandToSphere
} // Op.
} // UrFelt.

#endif /* INCLUDE_OP_ExpandToSphere_HPP_ */
