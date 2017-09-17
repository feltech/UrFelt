#include "UrSurface.hpp"

#include <algorithm>
#include <Urho3D/Physics/RigidBody.h>

#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"


namespace UrFelt
{


const Felt::Vec3f UrSurface::ray_miss = UrSurface::Surface::ray_miss;

UrSurface::UrSurface(
	const Felt::Vec3i& size_, const Felt::Vec3i& size_partition_, Urho3D::Node* pnode_
) :
	m_surface{size_, size_partition_},
	m_coll_shapes{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), nullptr
	},
	m_polys{m_surface},
	m_gpu_polys{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), GPUPoly{}
	},
	m_pnode{pnode_}, m_exit{false}, m_lock{false}, m_executor{&UrSurface::executor, this}
{
	for (
		Felt::PosIdx pos_idx_child = 0; pos_idx_child < m_polys.children().data().size();
		pos_idx_child++
	) {
		m_gpu_polys.get(pos_idx_child).bind(
			&m_polys.children().get(pos_idx_child), pnode_
		);
	}

	m_psurface_body = pnode_->CreateComponent<Urho3D::RigidBody>();
	m_psurface_body->SetKinematic(true);
	m_psurface_body->SetMass(10000000.0f);
	m_psurface_body->SetFriction(1.0f);
	m_psurface_body->SetUseGravity(false);
	m_psurface_body->SetRestitution(0.0);
	m_psurface_body->Activate();
}


UrSurface::~UrSurface()
{
	m_exit = true;
	m_lock.clear(std::memory_order_release);
	m_executor.join();
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


void UrSurface::enqueue(UrSurface::UpdateFn&& fn_)
{
	m_queue_updates.push([
		this,
		fn_ = std::move(fn_)
	]() {
		m_surface.update(std::move(fn_));
		m_polys.notify();
	});
}


void UrSurface::enqueue(
	const Felt::Vec3i& pos_leaf_lower_, const Felt::Vec3i& pos_leaf_upper_,
	UrSurface::UpdateFn&& fn_
) {
	m_queue_updates.push([
		this,
		fn_ = std::move(fn_),
		&pos_leaf_lower_ ,
		&pos_leaf_upper_
	]() {
		m_surface.update(pos_leaf_lower_, pos_leaf_upper_, std::move(fn_));
		m_polys.notify();
	});
}


void UrSurface::wake()
{
	m_lock.clear();
}


void UrSurface::executor()
{
	while (not m_exit)
	{
		while (m_lock.test_and_set(std::memory_order_acquire))
		{}

		while (not m_queue_updates.empty())
		{
			m_queue_updates.front()();
			m_queue_updates.pop();
		}
	}
}

} // UrFelt.

