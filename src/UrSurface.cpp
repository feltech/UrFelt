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
	m_pnode{pnode_}, m_exit{false}, m_executor{&UrSurface::executor, this}
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
	{
		std::lock_guard<std::mutex> lock(m_mutex_executor);
		m_exit = true;
	}
	wake();
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


void UrSurface::enqueue(UrSurface::Op::Base* op_)
{
	m_queue_executor.push_back(op_);
}


void UrSurface::executor()
{
	while (not m_exit)
	{
		std::unique_lock<std::mutex> lock(m_mutex_executor);
		m_execution.wait(lock);

		for (UrSurface::Op::Base* op : m_queue_executor)
			op->execute(*this);
	}
}


void UrSurface::await()
{
	std::lock_guard<std::mutex> lock(m_mutex_executor);
	for (UrSurface::Op::Base* op : m_queue_executor)
		op->callback();
	m_queue_executor.clear();
}


void UrSurface::wake()
{
	m_execution.notify_one();
}


UrSurface::Op::Base::Base(sol::function callback_) :
	callback{callback_}
{}


UrSurface::Op::Polygonise::Polygonise(sol::function callback_) :
	UrSurface::Op::Base{callback_}
{}


void UrSurface::Op::Polygonise::execute(UrSurface& surface)
{
	surface.polygonise();
}


UrSurface::Op::Simple::Simple(const float amount_, sol::function callback_) :
	UrSurface::Op::Base{callback_}, m_amount{amount_}
{}


void UrSurface::Op::Simple::execute(UrSurface& surface)
{
	surface.update([amount=m_amount](const auto&, const auto&) {
		return amount;
	});
}



} // UrFelt.

