#ifndef SRC_URSURFACECOLLISIONSHAPE_HPP_
#define SRC_URSURFACECOLLISIONSHAPE_HPP_

#include <Urho3D/Physics/CollisionShape.h>
#include "Common.hpp"

namespace UrFelt
{

class URHO3D_API UrSurfaceCollisionShape : public Urho3D::CollisionShape
{
    URHO3D_OBJECT(UrSurfaceCollisionShape, CollisionShape);

protected:
    const IsoGrid*			m_pisogrid;
    const IsoGrid::Child*	m_pisogrid_child;
    const Polys::Child*		m_ppoly_child;

public:
	using Base = Urho3D::CollisionShape;
	enum ShapeType
	{
		SHAPE_SURFACE = 1983
	};

	using Base::CollisionShape;

	void SetSurface(
		const IsoGrid * pisogrid_, const IsoGrid::Child * pisogrid_child_,
		const Polys::Child * ppoly_child_
	);

protected:
	btCollisionShape* UpdateDerivedShape(
        int shapeType, const Urho3D::Vector3& newWorldScale
    ) override;
};

} /* namespace Felt */

#endif /* SRC_URSURFACECOLLISIONSHAPE_HPP_ */
