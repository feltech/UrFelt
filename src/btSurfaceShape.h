#ifndef BT_SURFACE_SHAPE_H
#define BT_SURFACE_SHAPE_H

#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionShapes/btConcaveShape.h>
#include "UrSurface3D.hpp"

///The btSurfaceShape wraps a felt::Surface.
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

	~btSurfaceShape();


	void getAabb(const btTransform& t,btVector3& aabbMin,btVector3& aabbMax) const;

	void processAllTriangles(
		btTriangleCallback* callback,const btVector3& aabbMin,const btVector3& aabbMax
) const;

	void calculateLocalInertia(btScalar mass,btVector3& inertia) const;

	void setLocalScaling(const btVector3& scaling);
	const btVector3& getLocalScaling() const;

	//debugging
	const char* getName() const { return "FELTSURFACE"; }

	int	calculateSerializeBufferSize() const;

	///fills the dataBuffer and returns the struct name (and 0 on failure)
	const char* serialize(void* dataBuffer, btSerializer* serializer) const;

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

/// Do not change those serialization structures, it requires an updated
/// sBulletDNAstr/sBulletDNAstr64
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
SIMD_FORCE_INLINE const char* btSurfaceShape::serialize(
	void* dataBuffer, btSerializer* serializer
) const {
	btSurfaceShapeData* data = (btSurfaceShapeData*) dataBuffer;
	btCollisionShape::serialize(&data->m_collisionShapeData, serializer);

	return "btSurfaceShapeData";
}

#endif //BT_STATIC_PLANE_SHAPE_H



