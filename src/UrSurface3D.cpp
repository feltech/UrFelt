/*
 * UrSurface3D.cpp
 *
 *  Created on: 28 Jul 2015
 *      Author: dave
 */

#include "UrSurface3D.hpp"

#include <algorithm>

#include "FeltCollisionShape.hpp"
#include "btFeltCollisionConfiguration.hpp"


namespace felt
{

UrSurface3D::UrSurface3D(
	Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_, const VecDu& dims_,
	const VecDu& dims_partition_
) : Base(), m_physics_init(0) {
	this->init(pcontext_, pnode_root_, dims_, dims_partition_);
}

UrSurface3D::~UrSurface3D()
{}

void UrSurface3D::init(
	Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_,
	const VecDu& dims_, const VecDu& dims_partition_
) {
	m_pnode = pnode_root_;
	Base::init(dims_, dims_partition_);
	m_poly.init(*this, pcontext_, pnode_root_);
}

bool UrSurface3D::addPhysics(UINT chunk_size)
{
	UINT end_idx = std::min(
		m_physics_init + chunk_size, (UINT)this->m_grid_isogrid.children().data().size()
	);

	for (; m_physics_init < end_idx; m_physics_init++)
	{
		const Vec3i& pos_child = this->m_grid_isogrid.children().index(m_physics_init);
		FeltCollisionShape* shape = m_pnode->CreateComponent<FeltCollisionShape>();
		shape->SetSurface(this, pos_child);
	}

	return m_physics_init == this->m_grid_isogrid.children().data().size();
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
