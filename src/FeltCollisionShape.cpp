#include <Urho3D/Physics/PhysicsUtils.h>

#include "FeltCollisionShape.hpp"
#include "btSurfaceShape.h"

namespace felt
{
void FeltCollisionShape::SetSurface(const UrSurface3D* psurface_, const Vec3i& pos_child_)
{
	m_pos_child = pos_child_;
	m_psurface = psurface_;
	Vec3i child_size = m_psurface->isogrid().child_size().template cast<INT>();
	Vec3i pos = (child_size.array() * m_pos_child.array()).matrix();

	SetPosition(reinterpret_cast<Urho3D::Vector3&>(pos));
    SetShapeType((Urho3D::ShapeType)SHAPE_SURFACE);
}


void FeltCollisionShape::UpdateCustomShape(
	 int shapeType, btCollisionShape** ppshape, const Urho3D::Vector3& newWorldScale
) {
	*ppshape = new btSurfaceShape(m_psurface, m_pos_child);
	(*ppshape)->setLocalScaling(Urho3D::ToBtVector3(newWorldScale));
}

} /* namespace felt */
