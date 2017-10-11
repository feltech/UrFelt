#ifndef INCLUDE_OP_ExpandToBox_HPP_
#define INCLUDE_OP_ExpandToBox_HPP_

#include <Op/Base.hpp>
#include <UrSurface.hpp>

namespace UrFelt
{
namespace Op
{
namespace ExpandToBox
{


struct Impl : Base
{
	template <typename... Bounds>
	void execute(UrSurface& surface, Bounds... args);

	bool is_complete();
protected:
	Impl() = delete;
	Impl(
		const Urho3D::Vector3& pos_box_min_, const Urho3D::Vector3& pos_box_max_,
		sol::function callback_
	);

private:
	bool	m_is_complete;
	const Felt::Vec3f	m_pos_box_min;
	const Felt::Vec3f	m_pos_box_max;
	const Felt::Vec3f	m_pos_centre;
	Felt::Vec3f	m_pos_COM;
	Felt::ListIdx	m_size;
	std::vector<Surface::Plane>	m_planes;
};


struct Local : Impl, Bounded
{
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
		const Urho3D::Vector3& pos_box_min_, const Urho3D::Vector3& pos_box_max_
	);
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
		const Urho3D::Vector3& pos_box_min_, const Urho3D::Vector3& pos_box_max_,
		sol::function callback_
	);
	void execute(UrSurface& surface);
};


struct Global : Impl
{
	Global(const Urho3D::Vector3& pos_box_min_, const Urho3D::Vector3& pos_box_max_);
	Global(
		const Urho3D::Vector3& pos_box_min_, const Urho3D::Vector3& pos_box_max_,
		sol::function callback_
	);
	void execute(UrSurface& surface);
};



} // ExpandToBox
} // Op.
} // UrFelt.

#endif /* INCLUDE_OP_ExpandToBox_HPP_ */
