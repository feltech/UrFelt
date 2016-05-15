/*
 * UrSurface3D.cpp
 *
 *  Created on: 28 Jul 2015
 *      Author: dave
 */

#include "UrSurface3D.hpp"

namespace felt
{

UrSurface3D::UrSurface3D(
	Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_, const VecDu& dims_,
	const VecDu& dims_partition_
) : Base() {
	this->init(pcontext_, pnode_root_, dims_, dims_partition_);
}

UrSurface3D::~UrSurface3D()
{}

void UrSurface3D::init(
	Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_,
	const VecDu& dims_, const VecDu& dims_partition_
) {
	Base::init(dims_, dims_partition_);
	m_poly.init(*this, pcontext_, pnode_root_);
}

const UrPolyGrid3D& UrSurface3D::poly() const
{
	return m_poly;
}

UrPolyGrid3D& UrSurface3D::poly()
{
	return m_poly;
}

void UrSurface3D::update(std::function<FLOAT(const VecDi&, const IsoGrid&)> fn_)
{
	Base::update(fn_);
	m_poly.notify(*this);
}

} /* namespace felt */
