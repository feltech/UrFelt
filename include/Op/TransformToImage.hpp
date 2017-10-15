#ifndef INCLUDE_OP_ExpandToImage_HPP_
#define INCLUDE_OP_ExpandToImage_HPP_

#include <Op/Base.hpp>
#include <UrSurface.hpp>
#define cimg_display 0
#include <CImg.h>


namespace UrFelt
{
namespace Op
{
namespace TransformToImage
{


struct Impl : Base
{
	template <typename... Bounds>
	void execute(UrSurface& surface, Bounds... args);

	bool is_complete();
protected:
	Impl() = delete;
	Impl(
		const std::string& file_name_, const float ideal_, const float tolerance_,
		const float curvature_weight_, sol::function callback_
	);

private:
	bool m_is_complete;
	const std::string m_file_name;
	const float m_ideal;
	const float m_tolerance;
	const float m_curvature_weight;
	float m_divisor;
	cimg_library::CImg<float>	m_image;
};


struct Local : Impl, Bounded
{
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
		const std::string& file_name_, const float ideal_, const float tolerance_,
		const float curvature_weight_
	);
	Local(
		const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
		const std::string& file_name_, const float ideal_, const float tolerance_,
		const float curvature_weight_, sol::function callback_
	);
	void execute(UrSurface& surface);
};


struct Global : Impl
{
	Global(
		const std::string& file_name_, const float ideal_, const float tolerance_,
		const float curvature_weight_
	);
	Global(
		const std::string& file_name_, const float ideal_, const float tolerance_,
		const float curvature_weight_, sol::function callback_
	);
	void execute(UrSurface& surface);
};


} // ExpandToImage
} // Op.
} // UrFelt.

#endif /* INCLUDE_OP_ExpandToImage_HPP_ */
