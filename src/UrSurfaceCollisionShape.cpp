#include "UrSurfaceCollisionShape.hpp"

#include <Urho3D/Physics/PhysicsUtils.h>

#include "btSurfaceShape.h"


namespace UrFelt
{

void UrSurfaceCollisionShape::SetSurface(
	const UrFelt::IsoGrid *const pisogrid_, const UrFelt::IsoGrid::Child *const pisogrid_child_,
	const UrFelt::Polys::Child *const ppoly_child_
) {
	m_pisogrid = pisogrid_;
	m_pisogrid_child = pisogrid_child_;
	m_ppoly_child = ppoly_child_;
    SetShapeType((Urho3D::ShapeType)SHAPE_SURFACE);
}


btCollisionShape* UrSurfaceCollisionShape::UpdateDerivedShape(
	 int shapeType, const Urho3D::Vector3& newWorldScale
) {
	btCollisionShape* pshape = new btSurfaceShape(m_pisogrid, m_pisogrid_child, m_ppoly_child);
	pshape->setLocalScaling(Urho3D::ToBtVector3(newWorldScale));
	return pshape;
}

} /* namespace Felt */
