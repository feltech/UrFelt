/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btSurfaceShape.h"

#include <Urho3D/ThirdParty/Bullet/LinearMath/btTransformUtil.h>


btSurfaceShape::btSurfaceShape(const felt::UrSurface3D* psurface_, const felt::Vec3i& pos_child_)
: m_localScaling(btScalar(1.),btScalar(1.),btScalar(1.)), m_psurface(psurface_),
  m_pos_child(pos_child_)
{
	m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
}


btSurfaceShape::~btSurfaceShape()
{
}


void btSurfaceShape::getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const
{
	const felt::UrSurface3D::PhiGrid::Child& child = m_psurface->phi().child(m_pos_child);
	felt::Vec3i pos_min = child.offset();
	felt::Vec3i pos_max = pos_min + child.dims().template cast<felt::INT>();

	aabbMin.setValue(btScalar(pos_min(0)),btScalar(pos_min(1)),btScalar(pos_min(2)));
	aabbMax.setValue(btScalar(pos_max(0)),btScalar(pos_max(1)),btScalar(pos_max(2)));
}


void btSurfaceShape::processAllTriangles(
	btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax
) const {
	
	using Polys = felt::UrPolyGrid3D::PolyLeaf;
	using Vertex = felt::UrPoly3D::Vertex;
	const Polys& polys = m_psurface->poly().get(m_pos_child);
	
	for (unsigned idx = 0; idx < polys.spx().size(); idx++)
	{
		const felt::UrPoly3D::Simplex& spx = polys.spx()[idx];

		const Vertex& vtx0 =  polys.vtx()[spx.idxs[0]];
		const Vertex& vtx1 =  polys.vtx()[spx.idxs[1]];
		const Vertex& vtx2 =  polys.vtx()[spx.idxs[2]];

		btVector3 triangle[3];
		triangle[0] = btVector3(vtx0.pos(0), vtx0.pos(1), vtx0.pos(2));
		triangle[1] = btVector3(vtx1.pos(0), vtx1.pos(1), vtx1.pos(2));
		triangle[2] = btVector3(vtx2.pos(0), vtx2.pos(1), vtx2.pos(2));

		callback->processTriangle(triangle,0,idx);
	}
}


void btSurfaceShape::calculateLocalInertia(btScalar mass, btVector3& inertia) const
{
	(void)mass;

	//moving concave objects not supported
	
	inertia.setValue(btScalar(0.),btScalar(0.),btScalar(0.));
}


void btSurfaceShape::setLocalScaling(const btVector3& scaling)
{
	m_localScaling = scaling;
}


const btVector3& btSurfaceShape::getLocalScaling() const
{
	return m_localScaling;
}
