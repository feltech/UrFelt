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
	template <typename... Bounds>
	void execute(UrSurface& surface, Bounds... args);

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


struct Local : Impl, Bounded
{
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
		const Urho3D::Vector3& pos_centre_, const float radius_
	);
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
		const Urho3D::Vector3& pos_centre_, const float radius_,
		sol::function callback_
	);
	void execute(UrSurface& surface);
};


struct Global : Impl
{
	Global(const Urho3D::Vector3& pos_centre_, const float radius_);
	Global(const Urho3D::Vector3& pos_centre_, const float radius_, sol::function callback_);
	void execute(UrSurface& surface);
};


} // ExpandToSphere
} // Op.
} // UrFelt.

#endif /* INCLUDE_OP_ExpandToSphere_HPP_ */
