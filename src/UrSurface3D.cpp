#include "UrSurface3D.hpp"

#include <algorithm>

#include "FeltCollisionShape.hpp"
#include "btFeltCollisionConfiguration.hpp"


namespace felt
{

UrSurface3D::UrSurface3D(
	Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_, const VecDu& dims_,
	const VecDu& dims_partition_
) : Base()
{
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
	m_grid_coll_shapes = CollShapeGrid(
		this->m_grid_isogrid.children().size(), this->m_grid_isogrid.children().offset(), nullptr
	);
}

const UrPolyGrid3D& UrSurface3D::poly() const
{
	return m_poly;
}

UrPolyGrid3D& UrSurface3D::poly()
{
	return m_poly;
}


void UrSurface3D::flush()
{
	for (const Vec3i& pos_child : m_poly.changes().list())
	{
		const bool has_surface = this->layer(pos_child, 0).size() != 0;
		FeltCollisionShape* pshape = m_grid_coll_shapes.get(pos_child);

		if (has_surface && pshape == nullptr)
		{
			pshape = m_pnode->CreateComponent<FeltCollisionShape>();
			pshape->SetSurface(this, pos_child);
			m_grid_coll_shapes.get(pos_child) = pshape;
		}
		else if (!has_surface && pshape != nullptr)
		{
			m_pnode->RemoveComponent(pshape);
			m_grid_coll_shapes.get(pos_child) = nullptr;
		}
	}

	m_poly.update_gpu();
	m_poly.update_end();
}


void UrSurface3D::update(std::function<FLOAT(const VecDi&, const IsoGrid&)> fn_)
{
	Base::update(fn_);
	m_poly.notify(*this);
}

void  UrSurface3D::update(
	const VecDi& pos_leaf_lower_, const VecDi& pos_leaf_upper_,
	std::function<FLOAT(const VecDi&, const IsoGrid&)> fn_
) {
	Base::update(pos_leaf_lower_, pos_leaf_upper_, fn_);
	m_poly.notify(*this);
}

} /* namespace felt */
