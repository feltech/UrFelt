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
	m_initWatcher = boost::coroutines::coroutine<FLOAT>::pull_type{
		std::bind(&UrSurface3D::init_physics_task, this, std::placeholders::_1)
	};
}

FLOAT UrSurface3D::init_physics_chunk()
{
	m_initWatcher();
	if (!m_initWatcher)
		return 1;
	return m_initWatcher.get();
}

void UrSurface3D::init_physics_task(boost::coroutines::coroutine<FLOAT>::push_type& sink)
{
	const UINT num_children = this->m_grid_isogrid.children().data().size();
	for (UINT child_idx = 0; child_idx < num_children; child_idx++)
	{
		const Vec3i& pos_child = this->m_grid_isogrid.children().index(child_idx);
		FeltCollisionShape* shape = m_pnode->CreateComponent<FeltCollisionShape>();
		shape->SetSurface(this, pos_child);

		sink((FLOAT)child_idx / num_children);
	}
}

void UrSurface3D::init_physics(const UINT child_idx)
{
	const Vec3i& pos_child = this->m_grid_isogrid.children().index(child_idx);
	FeltCollisionShape* shape = m_pnode->CreateComponent<FeltCollisionShape>();
	shape->SetSurface(this, pos_child);
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
