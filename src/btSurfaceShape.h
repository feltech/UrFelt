/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2009 Erwin Coumans  http://bulletphysics.org

This sobtware is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this sobtware.
Permission is granted to anyone to use this sobtware for any purpose,
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this sobtware must not be misrepresented; you must not claim that you wrote the original sobtware. If you use this sobtware in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original sobtware.
3. This notice may not be removed or altered from any source distribution.
*/

#ifndef BT_STATIC_PLANE_SHAPE_H
#define BT_STATIC_PLANE_SHAPE_H

#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionShapes/btConcaveShape.h>
#include "UrSurface3D.hpp"

///The btSurfaceShape simulates an infinite non-moving (static) collision plane.
ATTRIBUTE_ALIGNED16(class) btSurfaceShape : public btConcaveShape
{
protected:

	btVector3	m_localAabbMin;
	btVector3	m_localAabbMax;
	btVector3	m_localScaling;
	const felt::UrSurface3D*    m_psurface;
	const felt::Vec3i         m_pos_child;

public:
	BT_DECLARE_ALIGNED_ALLOCATOR();

	btSurfaceShape(const felt::UrSurface3D* psurface_, const felt::Vec3i& pos_child);

	virtual ~btSurfaceShape();


	virtual void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const;

	virtual void	processAllTriangles(btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax) const;

	virtual void	calculateLocalInertia(btScalar mass,btVector3& inertia) const;

	virtual void	setLocalScaling(const btVector3& scaling);
	virtual const btVector3& getLocalScaling() const;
	
	//debugging
	virtual const char*	getName()const {return "FELTSURFACE";}

	virtual	int	calculateSerializeBufferSize() const;

	///fills the dataBuffer and returns the struct name (and 0 on failure)
	virtual	const char*	serialize(void* dataBuffer, btSerializer* serializer) const;

	const felt::UrSurface3D* surface () const
	{
		return m_psurface;
	}

	const felt::UrSurface3D::PosArray& layer (const felt::UINT& layerID) const
	{
		return m_psurface->layer(m_pos_child, layerID);
	}

    const felt::UrSurface3D::IsoGrid::Child& child () const
    {
        return m_psurface->isogrid().children().get(m_pos_child);
    }
};

///do not change those serialization structures, it requires an updated sBulletDNAstr/sBulletDNAstr64
struct	btSurfaceShapeData
{
	btCollisionShapeData	m_collisionShapeData;
	btVector3FloatData	m_localScaling;
	char	m_pad[4];
};


SIMD_FORCE_INLINE	int	btSurfaceShape::calculateSerializeBufferSize() const
{
	return sizeof(btSurfaceShapeData);
}

///fills the dataBuffer and returns the struct name (and 0 on failure)
SIMD_FORCE_INLINE	const char*	btSurfaceShape::serialize(void* dataBuffer, btSerializer* serializer) const
{
	btSurfaceShapeData* planeData = (btSurfaceShapeData*) dataBuffer;
	btCollisionShape::serialize(&planeData->m_collisionShapeData,serializer);

	return "btSurfaceShapeData";
}


#endif //BT_STATIC_PLANE_SHAPE_H



