#include <Op/ExpandByConstant.hpp>

namespace UrFelt
{
namespace Op
{
namespace ExpandByConstant
{


Impl::Impl(
	const float amount_, sol::function callback_
) :
	Base{callback_}, m_amount{amount_}
{}


template <typename... Bounds>
void Impl::execute(UrSurface& surface_, Bounds... bounds_)
{
	if (this->m_cancelled)
		return;

	const float amount = std::min(std::abs(m_amount), 1.0f) * Felt::sgn(m_amount);
	m_amount -= amount;
	surface_.update(bounds_..., [amount](const auto&, const auto&) {
		return amount;
	});
}


bool Impl::is_complete()
{
	return std::abs(m_amount) < std::numeric_limits<float>::epsilon();
}


Local::Local(
	const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
	const float amount_
) :
	Local{pos_min_, pos_max_, amount_, sol::function{}}
{}


Local::Local(
	const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_,
	const float amount_, sol::function callback_
) :
	Impl{amount_, callback_}, Bounded{pos_min_, pos_max_}
{}


void Local::execute(UrSurface& surface)
{
	Impl::execute(surface, m_pos_min, m_pos_max);
}


Global::Global(const float amount_) :
	Global{amount_, sol::function{}}
{}


Global::Global(
	const float amount_, sol::function callback_
) :
	Impl{amount_, callback_}
{}


void Global::execute(UrSurface& surface)
{
	Impl::execute(surface);
}


} // ExpandByConstant.
} // Op.
} // UrFelt.



