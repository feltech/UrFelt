#ifndef INCLUDE_OP_EXPANDBYCONSTANT_HPP_
#define INCLUDE_OP_EXPANDBYCONSTANT_HPP_

#include <Op/Base.hpp>
#include <UrSurface.hpp>

namespace UrFelt
{
namespace Op
{
namespace ExpandByConstant
{


struct Impl : Base
{
	template <typename... Bounds>
	void execute(UrSurface& surface, Bounds... args);

	bool is_complete();
protected:
	Impl() = delete;
	Impl(const float amount_, sol::function);

private:
	float m_amount;
};


struct Local : Impl, Bounded
{
	Local(const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_, const float amount_);
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_, const float amount_,
		sol::function callback_
	);
	void execute(UrSurface& surface);
};


struct Global : Impl
{
	Global(const float amount_);
	Global(const float amount_, sol::function callback_);
	void execute(UrSurface& surface);
};



} // ExpandByConstant
} // Op.
} // UrFelt.

#endif /* INCLUDE_OP_EXPANDBYCONSTANT_HPP_ */
