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


btSurfaceShape::btSurfaceShape (
	const UrFelt::IsoGrid *const pisogrid_, const UrFelt::IsoGrid::Child *const pisogrid_child_,
	const UrFelt::Polys::Child *const ppoly_child_
) :
	m_localScaling{btScalar{1.}, btScalar{1.}, btScalar{1.}},
	m_pisogrid{pisogrid_}, m_pisogrid_child{pisogrid_child_}, m_ppoly_child{ppoly_child_}
{
	this->m_shapeType = CUSTOM_CONCAVE_SHAPE_TYPE;
}


void btSurfaceShape::getAabb (const btTransform& t, btVector3& aabbMin, btVector3& aabbMax) const
{
	const Felt::Vec3i& pos_min = m_pisogrid_child->offset();
	const Felt::Vec3i& pos_max = pos_min + m_pisogrid_child->size();

	aabbMin.setValue(btScalar(pos_min(0)),btScalar(pos_min(1)),btScalar(pos_min(2)));
	aabbMax.setValue(btScalar(pos_max(0)),btScalar(pos_max(1)),btScalar(pos_max(2)));
}


void btSurfaceShape::processAllTriangles (
	btTriangleCallback* callback, const btVector3& aabbMin, const btVector3& aabbMax
) const {
	
	using Vertex = UrFelt::Polys::Child::Vertex;
	using Simplex = UrFelt::Polys::Child::Simplex;
	
	for (unsigned idx = 0; idx < m_ppoly_child->spxs().size(); idx++)
	{
		const Simplex& spx = m_ppoly_child->spxs()[idx];

		const Vertex& vtx0 =  m_ppoly_child->vtxs()[spx.idxs[0]];
		const Vertex& vtx1 =  m_ppoly_child->vtxs()[spx.idxs[1]];
		const Vertex& vtx2 =  m_ppoly_child->vtxs()[spx.idxs[2]];

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
