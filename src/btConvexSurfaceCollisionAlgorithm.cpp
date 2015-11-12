/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btConvexSurfaceCollisionAlgorithm.h"

#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionShapes/btConvexShape.h>
#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h>

#include "btSurfaceShape.h"

//#include <stdio.h>

btConvexSurfaceCollisionAlgorithm::btConvexSurfaceCollisionAlgorithm(
	btPersistentManifold* mf, const btCollisionAlgorithmConstructionInfo& ci,
	const btCollisionObjectWrapper* col0Wrap, const btCollisionObjectWrapper* col1Wrap,
	bool isSwapped, int numPerturbationIterations, int minimumPointsPerturbationThreshold
)
: btCollisionAlgorithm(ci),
m_ownManifold(false),
m_manifoldPtr(mf),
m_isSwapped(isSwapped),
m_numPerturbationIterations(numPerturbationIterations),
m_minimumPointsPerturbationThreshold(minimumPointsPerturbationThreshold)
{
	const btCollisionObjectWrapper* convexObjWrap = m_isSwapped? col1Wrap : col0Wrap;
	const btCollisionObjectWrapper* surfaceObjWrap = m_isSwapped? col0Wrap : col1Wrap;

	if (
		!m_manifoldPtr
		&& m_dispatcher->needsCollision(
			convexObjWrap->getCollisionObject(), surfaceObjWrap->getCollisionObject()
		)
	) {
		m_manifoldPtr = m_dispatcher->getNewManifold(
			convexObjWrap->getCollisionObject(), surfaceObjWrap->getCollisionObject()
		);
		m_ownManifold = true;
	}
}


btConvexSurfaceCollisionAlgorithm::~btConvexSurfaceCollisionAlgorithm()
{
	if (m_ownManifold)
	{
		if (m_manifoldPtr)
			m_dispatcher->releaseManifold(m_manifoldPtr);
	}
}


void btConvexSurfaceCollisionAlgorithm::processCollision (
	const btCollisionObjectWrapper* body0Wrap, const btCollisionObjectWrapper* body1Wrap,
	const btDispatcherInfo& dispatchInfo, btManifoldResult* resultOut
) {
	(void)dispatchInfo;
	if (!m_manifoldPtr)
		return;

	const btCollisionObjectWrapper* convexObjWrap = m_isSwapped? body1Wrap : body0Wrap;
	const btCollisionObjectWrapper* surfaceObjWrap = m_isSwapped? body0Wrap: body1Wrap;

	btConvexShape* convexShape = (btConvexShape*) convexObjWrap->getCollisionShape();
	btSurfaceShape* surfaceShape = (btSurfaceShape*) surfaceObjWrap->getCollisionShape();

	btTransform surfaceInConvex = (
		convexObjWrap->getWorldTransform().inverse() * surfaceObjWrap->getWorldTransform()
	);
	btTransform convexInSurfaceTrans = (
		surfaceObjWrap->getWorldTransform().inverse() * convexObjWrap->getWorldTransform()
	);

	resultOut->setPersistentManifold(m_manifoldPtr);

	const float X = (float)surfaceShape->surface()->phi().dims()(0);
	const float Y = (float)surfaceShape->surface()->phi().dims()(1);

	std::set<float> vtx_hash_set;

	m_manifoldPtr->clearManifold();

	for (const felt::Vec3i& pos_leaf : surfaceShape->layer(0))
	{
		const felt::Vec3f& grad = surfaceShape->surface()->phi().grad(pos_leaf);
		const felt::FLOAT& mag_grad_sq = grad.blueNorm();
		if (mag_grad_sq < 0.1f)
			continue;
		const felt::Vec3f& normal = grad.normalized();

		btVector3 btnormal(normal(0), normal(1), normal(2));

		btVector3 btnormalInConvex = surfaceInConvex.getBasis() * (-btnormal);

		btVector3 vtx = convexShape->localGetSupportingVertexWithoutMargin(btnormalInConvex);

		const float& vtx_hash = vtx.z() * X * Y + vtx.y() * X + vtx.x();
		if (!vtx_hash_set.insert(vtx_hash).second)
			continue;

		btVector3 vtxInSurface = convexInSurfaceTrans(vtx);

		const felt::Vec3f pos_vtx(vtxInSurface.x(), vtxInSurface.y(), vtxInSurface.z());

		if (!surfaceShape->surface()->phi().inside(pos_vtx))
			continue;

		btScalar distance = surfaceShape->surface()->phi().interp(pos_vtx);

		if (distance < m_manifoldPtr->getContactBreakingThreshold())
		{
			btVector3 normalOnSurfaceB = surfaceObjWrap->getWorldTransform().getBasis() * btnormal;
			resultOut->addContactPoint(normalOnSurfaceB, vtxInSurface, distance);
		}
	}


	if (m_ownManifold)
	{
		if (m_manifoldPtr->getNumContacts())
		{
			resultOut->refreshContactPoints();
		}
	}
}

btScalar btConvexSurfaceCollisionAlgorithm::calculateTimeOfImpact(
	btCollisionObject* col0, btCollisionObject* col1,
	const btDispatcherInfo& dispatchInfo,btManifoldResult* resultOut
) {
	(void)resultOut;
	(void)dispatchInfo;
	(void)col0;
	(void)col1;

	//not yet
	return btScalar(1.);
}
