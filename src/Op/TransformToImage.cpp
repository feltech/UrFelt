#include <Urho3D/IO/Log.h>
#include "Op/TransformToImage.hpp"


namespace UrFelt
{
namespace Op
{
namespace TransformToImage
{


Impl::Impl(
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_, sol::function callback_
) :
	Base{callback_},
	m_is_complete{false}, m_file_name{file_name_}, m_ideal{ideal_}, m_tolerance{tolerance_},
	m_curvature_weight{curvature_weight_}, m_image{}, m_divisor{0}
{}


template <typename... Bounds>
void Impl::execute(UrSurface& surface_, Bounds... bounds_)
{
	m_is_complete = true;

	if (this->m_cancelled)
		return;

	if (!m_image.size())
	{
		m_image.load(m_file_name.c_str());
		m_divisor = m_image.max();
		std::stringstream ss;
		ss << "Loaded " << m_file_name << " with " << m_image.size() << " pixels in a " <<
			m_image.width() << "x" <<  m_image.height() << "x" <<  m_image.depth() <<
			" configuration with max voxel value " << m_divisor;
		URHO3D_LOGINFO(ss.str().c_str());
	}

	try
	{

	surface_.update(bounds_..., [this](const Felt::Vec3i& pos_, const IsoGrid& isogrid_) {
		using namespace Felt;
		// Size of surface update to consider zero (thus finished).
		static constexpr Distance epsilon = 0.0001;
		// Multiplier to ensure no surface update with magnitude greater than 1.0.
		const Distance clamp = 0.5f * 1.0f / (2*(1 + m_curvature_weight*2));

		const Distance voxel = m_image.atXYZ(
			pos_(0) + m_image.width()/2, pos_(1) + m_image.height()/2, pos_(2) + m_image.depth()/2,
			0, std::numeric_limits<unsigned char>::max()
		)/m_divisor;
		const Distance diff_sq = pow(voxel - m_ideal, 2);
		const Distance tolerance_sq = pow(m_tolerance, 2);
		const Distance speed = -1 + 2 * diff_sq / ( diff_sq + tolerance_sq);

		Distance amount;

		// Get entropy-satisfying gradient (surface "normal").
		const Vec3f& grad = isogrid_.gradE(pos_);
		// If the gradient is zero, we must have a singularity, so can't use gradient.
		if (grad.isZero())
		{
			amount = 0.5f * speed;
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

			const Distance curvature = isogrid_.curv(pos_);

			amount = clamp * grad_norm*(speed + m_curvature_weight*curvature);
		}

		m_is_complete &= std::abs(amount) <= epsilon;

		return amount;
	});

	}
	catch (const std::domain_error& e)
	{
		const unsigned int x_max = surface_.isogrid().size()(0);
		const unsigned int y_max = surface_.isogrid().size()(1);
		const unsigned int z_max = surface_.isogrid().size()(2);

		cimg_library::CImg<float> out(x_max, y_max, z_max);

		for (int x = 0; x < x_max; x++)
			for (int y = 0; y < y_max; y++)
				for (int z = 0; z < z_max; z++)
				{
					Felt::Vec3i pos{x,y,z};
					pos += surface_.isogrid().offset();
					out(x,y,z) = surface_.isogrid().get(pos);
				}

		out.save_analyze("/tmp/dump.hdr");
		std::cerr << "Dumped isogrid to /tmp/dump.hdr" << std::endl;

		throw e;
	}
}


bool Impl::is_complete()
{
	return m_is_complete;
}


Local::Local(
	const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_
) :
	Local{pos_min_, pos_max_, file_name_, ideal_, tolerance_, curvature_weight_, sol::function{}}
{}


Local::Local(
	const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_, sol::function callback_
) :
	Impl{file_name_, ideal_, tolerance_, curvature_weight_, callback_}, Bounded{pos_min_, pos_max_}
{}


void Local::execute(UrSurface& surface)
{
	Impl::execute(surface, m_pos_min, m_pos_max);
}


Global::Global(
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_
) :
	Global{file_name_, ideal_, tolerance_, curvature_weight_, sol::function{}}
{}


Global::Global(
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_, sol::function callback_
) :
	Impl{file_name_, ideal_, tolerance_, curvature_weight_, callback_}
{}


void Global::execute(UrSurface& surface)
{
	Impl::execute(surface);
}

} // ExpandToImage.
} // Op.
} // UrFelt.



