#include "UrSurface.hpp"

#include <algorithm>

#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"


namespace UrFelt
{


const Felt::Vec3f UrSurface::ray_miss = UrSurface::Surface::ray_miss;


UrSurface::UrSurface(
	const Felt::Vec3i& size_, const Felt::Vec3i& size_partition_,
	Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_
) :
	m_surface{size_, size_partition_},
	m_coll_shapes{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), nullptr
	},
	m_polys{m_surface},
	m_gpu_polys{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), GPUPoly{}
	},
	m_pnode{pnode_root_}
{
	for (
		Felt::PosIdx pos_idx_child = 0; pos_idx_child < m_polys.children().data().size();
		pos_idx_child++
	) {
		m_gpu_polys.get(pos_idx_child).bind(
			&m_polys.children().get(pos_idx_child), pcontext_, pnode_root_
		);
	}
}


void UrSurface::polygonise()
{
	m_polys.march();
}


void UrSurface::flush()
{
	for (const Felt::PosIdx pos_idx_child : m_polys.changes())
	{
		const bool has_surface = m_surface.is_intersected(pos_idx_child);
		UrSurfaceCollisionShape * pshape = m_coll_shapes.get(pos_idx_child);

		if (has_surface && pshape == nullptr)
		{
			pshape = m_pnode->CreateComponent<UrSurfaceCollisionShape>();
			pshape->SetSurface(
				&m_surface.isogrid(), &m_surface.isogrid().children().get(pos_idx_child),
				&m_polys.children().get(pos_idx_child)
			);
			m_coll_shapes.set(pos_idx_child, pshape);
		}
		else if (!has_surface && pshape != nullptr)
		{
			m_pnode->RemoveComponent(pshape);
			m_coll_shapes.set(pos_idx_child, nullptr);
		}

		m_gpu_polys.get(pos_idx_child).flush();
	}
}

} // UrFelt.
