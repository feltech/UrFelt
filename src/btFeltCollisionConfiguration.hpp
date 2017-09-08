/*
 * btFeltCollisionConfiguration.hpp
 *
 *  Created on: 8 Nov 2015
 *      Author: dave
 */

#ifndef SRC_BTFELTCOLLISIONCONFIGURATION_HPP_
#define SRC_BTFELTCOLLISIONCONFIGURATION_HPP_

#include <Urho3D/ThirdParty/Bullet/BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>

namespace Felt
{

class btFeltCollisionConfiguration : public btDefaultCollisionConfiguration
{
protected:
	btCollisionAlgorithmCreateFunc*	m_surfaceConvexCF;
	btCollisionAlgorithmCreateFunc*	m_convexSurfaceCF;

public:
	btFeltCollisionConfiguration ();
	~btFeltCollisionConfiguration ();

	btCollisionAlgorithmCreateFunc* getCollisionAlgorithmCreateFunc(int proxyType0, int proxyType1);
};

} /* namespace Felt */

#endif /* SRC_BTFELTCOLLISIONCONFIGURATION_HPP_ */

