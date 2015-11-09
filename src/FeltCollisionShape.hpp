#ifndef SRC_FELTCOLLISIONSHAPE_HPP_
#define SRC_FELTCOLLISIONSHAPE_HPP_

#include <Urho3D/Physics/CollisionShape.h>

#include "UrSurface3D.hpp"

namespace felt
{

class URHO3D_API FeltCollisionShape : public Urho3D::CollisionShape
{
    URHO3D_OBJECT(FeltCollisionShape, CollisionShape);

protected:
	const UrSurface3D*	m_psurface;
	Vec3i				m_pos_child;

public:
	using Base = Urho3D::CollisionShape;
	enum ShapeType
	{
		SHAPE_SURFACE = Urho3D::ShapeType::SHAPE_TERRAIN + 1
	};

	using Base::CollisionShape;

	void SetSurface(const UrSurface3D* psurface_, const felt::Vec3i& pos_child_);

protected:
	void UpdateCustomShape(
		 int shapeType, btCollisionShape** ppshape, const Urho3D::Vector3& newWorldScale
	);
};

} /* namespace felt */

#endif /* SRC_FELTCOLLISIONSHAPE_HPP_ */
