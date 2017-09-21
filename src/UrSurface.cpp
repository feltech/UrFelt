#include "UrSurface.hpp"

#include <algorithm>
#include <limits>

#include <Urho3D/Physics/RigidBody.h>

#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"


namespace UrFelt
{


const Felt::Vec3f UrSurface::ray_miss = UrSurface::Surface::ray_miss;


void UrSurface::to_lua(sol::table& lua)
{
	lua.new_usertype<UrSurface>(
		"UrSurface",
		sol::call_constructor,
		sol::constructors<UrSurface(const Felt::Vec3i&, const Felt::Vec3i&, Urho3D::Node*)>(),
		"seed", &UrSurface::seed,
		"invalidate", &UrSurface::invalidate,
		"polygonise", &UrSurface::polygonise,
		"flush", &UrSurface::flush,

		"update", [](UrSurface& self, sol::function fn_) {
			self.update([&fn_](const Felt::Vec3i& pos_, const UrSurface::IsoGrid& isogrid_) {
				Urho3D::IntVector3 vpos_ = reinterpret_cast<const Urho3D::IntVector3&>(pos_);
				Felt::Distance dist = fn_(vpos_, isogrid_);
				return dist;
			});
		},

		"enqueue", &UrSurface::enqueue,
		"await", &UrSurface::await,
		"poll", &UrSurface::poll
	);

	lua.new_usertype<UrSurface::IsoGrid>(
		"IsoGrid"
	);

	sol::table lua_Op = lua.create_named("Op");

	lua_Op.new_usertype<UrSurface::Op::Polygonise>(
		"Polygonise",
		sol::call_constructor,
		sol::constructors<
			UrSurface::Op::Polygonise(),
			UrSurface::Op::Polygonise(sol::function)
		>()
	);

	lua_Op.new_usertype<UrSurface::Op::ExpandByConstant>(
		"ExpandByConstant",
		sol::call_constructor,
		sol::constructors<
			UrSurface::Op::ExpandByConstant(float),
			UrSurface::Op::ExpandByConstant(float, sol::function)
		>()
	);
}


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
	m_pnode{pnode_}, m_exit{false}, m_queue_pending{}, m_queue_done{}
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

	m_executor = std::thread{&UrSurface::executor, this};
}


UrSurface::~UrSurface()
{
	m_exit = true;
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


void UrSurface::enqueue(const UrSurface::Op::Base& op_)
{
	std::lock_guard<boost::detail::spinlock> lock(m_lock_pending);
	m_queue_pending.push_back(op_.clone());
}


void UrSurface::executor()
{
	while (not m_exit)
	{
		UrSurface::Op::Base* op = nullptr;
		{
			Guard lock{m_lock_pending};
			if (not m_queue_pending.empty())
				op = m_queue_pending.front().get();
		}

		if (op == nullptr)
		{
			std::this_thread::yield();
			continue;
		}

		op->execute(*this);

		if (op->is_complete())
		{
			Guard lock_pending{m_lock_pending};
			Guard lock_done{m_lock_done};
			m_queue_done.push_back(std::move(m_queue_pending.front()));
			m_queue_pending.pop_front();
		}
		else
		{
			Guard lock{m_lock_pending};
			m_queue_pending.push_back(std::move(m_queue_pending.front()));
			m_queue_pending.pop_front();
		}
	}
}


void UrSurface::await()
{
	while (true)
	{
		Guard lock(m_lock_pending);
		if (m_queue_pending.empty())
		{
			Guard lock(m_lock_done);
			for (Op::Ptr& op : m_queue_done)
				if (op->callback)
					op->callback();
			m_queue_done.clear();
			return;
		}
		std::this_thread::yield();
	}
}

void UrSurface::poll()
{
	Guard lock(m_lock_done);
	for (Op::Ptr& op : m_queue_done)
		if (op->callback)
			op->callback();
	m_queue_done.clear();
}

UrSurface::Op::Base::Base(sol::function callback_) :
	callback{callback_}
{}


bool UrSurface::Op::Base::is_complete()
{
	return true;
}


UrSurface::Op::Polygonise::Polygonise(sol::function callback_) :
	UrSurface::Op::Base{callback_}
{}


void UrSurface::Op::Polygonise::execute(UrSurface& surface)
{
	surface.polygonise();
}


UrSurface::Op::ExpandByConstant::ExpandByConstant(const float amount_) :
	m_amount{amount_}
{}


UrSurface::Op::ExpandByConstant::ExpandByConstant(const float amount_, sol::function callback_) :
	UrSurface::Op::Base{callback_}, m_amount{amount_}
{}


void UrSurface::Op::ExpandByConstant::execute(UrSurface& surface)
{
	const float amount = std::min(std::abs(m_amount), 1.0f) * Felt::sgn(m_amount);
	m_amount -= amount;
	surface.update([amount](const auto&, const auto&) {
		return amount;
	});
}

bool UrSurface::Op::ExpandByConstant::is_complete()
{
	return std::abs(m_amount) < std::numeric_limits<float>::epsilon();
}


} // UrFelt.

