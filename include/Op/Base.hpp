#ifndef INCLUDE_OP_BASE_HPP_
#define INCLUDE_OP_BASE_HPP_

#include <Felt/Impl/Common.hpp>

#include <UrSurface.hpp>

#include <sol.hpp>
#include <Urho3D/Math/Vector3.h>


#define URFELT_URSURFACE_OP_FACTORY(Class)\
	template <typename... Args>\
	static Ptr<Class> factory(Args&&... args) \
	{ \
		return std::make_shared<Class>(std::forward<Args>(args)...); \
	}


namespace UrFelt
{

class UrSurface;

namespace Op
{

struct Base
{
	sol::function callback;
	virtual void execute(UrSurface& surface) = 0;
	virtual bool is_complete();
	virtual void stop();

	virtual ~Base() = default;
protected:
	Base() = delete;
	Base(sol::function callback_);
	bool m_cancelled;
};


struct Bounded
{
	Bounded(const Urho3D::Vector3& pos_min_, const Urho3D::Vector3& pos_max_);
protected:
	const Felt::Vec3i	m_pos_min;
	const Felt::Vec3i	m_pos_max;
};


using Ptr =  std::shared_ptr<Base>;

}
}
#endif /* INCLUDE_OP_BASE_HPP_ */
