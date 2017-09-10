/*
 * btFeltCollisionConfiguration.cpp
 *
 *  Created on: 8 Nov 2015
 *      Author: dave
 */

#include "btFeltCollisionConfiguration.hpp"
#include "btConvexSurfaceCollisionAlgorithm.h"

namespace UrFelt
{

btFeltCollisionConfiguration::btFeltCollisionConfiguration ()
: btDefaultCollisionConfiguration::btDefaultCollisionConfiguration()
{
	void* mem = btAlignedAlloc (sizeof(btConvexSurfaceCollisionAlgorithm::CreateFunc),16);
	m_convexSurfaceCF = new (mem) btConvexSurfaceCollisionAlgorithm::CreateFunc;
	mem = btAlignedAlloc (sizeof(btConvexSurfaceCollisionAlgorithm::CreateFunc),16);
	m_surfaceConvexCF = new (mem) btConvexSurfaceCollisionAlgorithm::CreateFunc;
	m_surfaceConvexCF->m_swapped = true;
}

btFeltCollisionConfiguration::~btFeltCollisionConfiguration ()
{
	m_convexSurfaceCF->~btCollisionAlgorithmCreateFunc();
	btAlignedFree( m_convexSurfaceCF);
	m_surfaceConvexCF->~btCollisionAlgorithmCreateFunc();
	btAlignedFree( m_surfaceConvexCF);
}


btCollisionAlgorithmCreateFunc* btFeltCollisionConfiguration::getCollisionAlgorithmCreateFunc(
	int proxyType0, int proxyType1
) {
	if (btBroadphaseProxy::isConvex(proxyType0) && (proxyType1 == CUSTOM_CONCAVE_SHAPE_TYPE))
	{
		return m_convexSurfaceCF;
	}

	if (btBroadphaseProxy::isConvex(proxyType1) && (proxyType0 == CUSTOM_CONCAVE_SHAPE_TYPE))
	{
		return m_surfaceConvexCF;
	}

	return btDefaultCollisionConfiguration::getCollisionAlgorithmCreateFunc(proxyType0, proxyType1);
}


} /* namespace Felt */


