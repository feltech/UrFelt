#include <vector>
#include <algorithm>
#include <limits>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/ThirdParty/toluapp/tolua++.h>

#include <Felt/Impl/Util.hpp>

#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"

#include "Op/Serialise.hpp"
#include "Op/Polygonise.hpp"
#include "Op/ExpandByConstant.hpp"
#include "Op/TransformToBox.hpp"
#include "Op/TransformToImage.hpp"
#include "Op/TransformToSphere.hpp"



namespace sol
{
	namespace stack
	{
	template <typename T>
	struct userdata_checker<extensible<T>> {
		template <typename Handler>
		static bool check(
			lua_State* L, int relindex, type index_type, Handler&& handler, record& tracking) {
			// just marking unused parameters for no compiler warnings
			(void)index_type;
			(void)handler;
			int index = lua_absindex(L, relindex);
			std::string name = sol::detail::short_demangle<T>();
			tolua_Error tolua_err;

			const bool is_tolua_usertype = tolua_isusertype(L, index, name.c_str(), 0, &tolua_err);
			if (is_tolua_usertype)
				tracking.use(1);
			return is_tolua_usertype;
		}
	};
	} // stack

template <>
struct usertype_traits<Urho3D::Vector3> {
	static const std::string& metatable() {
		static const std::string n{"Vector3"};
		return n;
	}
};
} // sol



namespace UrFelt
{

const Felt::Vec3f UrSurface::ray_miss = UrSurface::Surface::ray_miss;


void UrSurface::to_lua(sol::table& lua)
{
	lua.new_usertype<UrSurface>(
		"UrSurface",
		sol::call_constructor,
		sol::constructors<
			UrSurface(const Urho3D::IntVector3&, const Urho3D::IntVector3&, Urho3D::Node*)
		>(),

		"seed", &UrSurface::seed,
		"ray", &UrSurface::ray,
		"invalidate", &UrSurface::invalidate,
		"flush", &UrSurface::flush,
		"flush_graphics", &UrSurface::flush_graphics,

		"await", &UrSurface::await,
		"poll", &UrSurface::poll,

		"load", &Op::Serialise::Load::load,

		"save", sol::overload(
			&UrSurface::enqueue<
				Op::Serialise::Save,
				const std::string&
			>,
			&UrSurface::enqueue<
				Op::Serialise::Save,
				const std::string&,
				sol::function
			>
		),

		"polygonise", sol::overload(
			&UrSurface::enqueue<Op::Polygonise::Global>,
			&UrSurface::enqueue<Op::Polygonise::Global, sol::function>
		),

		"expand_by_constant", sol::overload(
			&UrSurface::enqueue<
				Op::ExpandByConstant::Global,
				const float
			>,
			&UrSurface::enqueue<
				Op::ExpandByConstant::Global,
				const float,
				sol::function
			>,
			&UrSurface::enqueue<
				Op::ExpandByConstant::Local,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				const float
			>,
			&UrSurface::enqueue<
				Op::ExpandByConstant::Local,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				const float,
				sol::function
			>
		),
		"attract_to_sphere", sol::overload(
			&UrSurface::enqueue<
				Op::TransformToSphere::Attract,
				const Urho3D::Vector3&, const float
			>,
			&UrSurface::enqueue<
				Op::TransformToSphere::Attract,
				const Urho3D::Vector3&, const float,
				sol::function
			>
		),
		"repel_from_sphere", sol::overload(
			&UrSurface::enqueue<
				Op::TransformToSphere::Repel,
				const Urho3D::Vector3&, const float
			>,
			&UrSurface::enqueue<
				Op::TransformToSphere::Repel,
				const Urho3D::Vector3&, const float,
				sol::function
			>
		),
		"transform_to_box", sol::overload(
			&UrSurface::enqueue<
				Op::TransformToBox::Global,
				const Urho3D::Vector3&, const Urho3D::Vector3&
			>,
			&UrSurface::enqueue<
				Op::TransformToBox::Global,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				sol::function
			>,
			&UrSurface::enqueue<
				Op::TransformToBox::Local,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				const Urho3D::Vector3&, const Urho3D::Vector3&
			>,
			&UrSurface::enqueue<
				Op::TransformToBox::Local,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				sol::function
			>
		),
		"transform_to_image", sol::overload(
			&UrSurface::enqueue<
				Op::TransformToImage::Global,
				const std::string&, const float, const float, const float
			>,
			&UrSurface::enqueue<
				Op::TransformToImage::Global,
				const std::string&, const float, const float, const float,
				sol::function
			>,
			&UrSurface::enqueue<
				Op::TransformToImage::Local,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				const std::string&, const float, const float, const float
			>,
			&UrSurface::enqueue<
				Op::TransformToImage::Local,
				const Urho3D::Vector3&, const Urho3D::Vector3&,
				const std::string&, const float, const float, const float,
				sol::function
			>
		),
		"stats", &UrSurface::stats
	);

	lua.new_usertype<Op::Base>(
		"Op",
		"new", sol::no_constructor,
		"stop", &Op::Base::stop
	);

	lua.new_usertype<Op::Serialise::Load>(
		"Load",
		"ready", &Op::Serialise::Load::ready,
		"get", &Op::Serialise::Load::get,
		"new", sol::no_constructor
	);

	lua.new_usertype<UrFelt::Surface::Stats>("Stats");
}


UrSurface::UrSurface(
	const Urho3D::IntVector3& size_, const Urho3D::IntVector3& size_partition_, Urho3D::Node* pnode_
) :
	UrSurface{
		reinterpret_cast<const Felt::Vec3i&>(size_),
		reinterpret_cast<const Felt::Vec3i&>(size_partition_),
		pnode_
	}
{}


UrSurface::UrSurface(
	const Felt::Vec3i& size_, const Felt::Vec3i& size_partition_, Urho3D::Node* pnode_
) :
	m_exit{false},
	m_lock_pending{}, m_lock_done{},
	m_queue_pending{}, m_queue_done{},
	m_surface{size_, size_partition_},
	m_polys{m_surface},
	m_coll_shapes{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), nullptr
	},
	m_gpu_polys{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), GPUPoly{}
	},
	m_pnode{pnode_}
{
	for (
		Felt::PosIdx pos_idx_child = 0; pos_idx_child < m_polys.children().data().size();
		pos_idx_child++
	) {
		m_gpu_polys.get(pos_idx_child).bind(
			&m_polys.children().get(pos_idx_child), pnode_
		);
	}
    create_surface_body(pnode_);
	m_executor = std::thread{&UrSurface::executor, this};
}


UrSurface::~UrSurface()
{
	m_exit = true;
	m_executor.join();
}


void UrSurface::executor()
{
	while (not m_exit)
	{
		if (m_pause)
		{
			std::this_thread::yield();
			continue;
		}

		Op::Base* op = nullptr;
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

		Guard lock{m_lock_pending};

		op->execute(*this);

		if (op->is_complete())
		{
			Guard lock_done{m_lock_done};
			m_queue_done.push_back(std::move(m_queue_pending.front()));
			m_queue_pending.pop_front();
		}
		else
		{
			m_queue_pending.push_back(std::move(m_queue_pending.front()));
			m_queue_pending.pop_front();
		}
	}
}


void UrSurface::await()
{
	while (true)
	{
		bool empty;
		{
			Pause pause{this};
			empty = m_queue_pending.empty();
		}

		if (!empty)
		{
			std::this_thread::yield();
			continue;
		}

		poll();
		return;
	}
}


void UrSurface::poll()
{
	std::deque<Op::Ptr> queue_done;
	{
		Guard lock(m_lock_done);
		queue_done = std::move(m_queue_done);
	}

	for (Op::Ptr& op : queue_done)
		if (op->callback)
			op->callback();
}


void UrSurface::flush()
{
	Pause pause{this};
	flush_physics_impl();
	flush_graphics_impl();
}


void UrSurface::flush_graphics()
{
	Pause pause{this};
	flush_graphics_impl();
}


sol::optional<Urho3D::Vector3> UrSurface::ray(const Urho3D::Ray& ray_) const
{
	const Felt::Vec3f& pos_hit = m_surface.ray(
		reinterpret_cast<const Felt::Vec3f&>(ray_.origin_),
		reinterpret_cast<const Felt::Vec3f&>(ray_.direction_)
	);
	if (pos_hit == ray_miss)
		return sol::optional<Urho3D::Vector3>{};
	const Urho3D::Vector3& pos_ur_hit = reinterpret_cast<const Urho3D::Vector3&>(pos_hit);
	return sol::optional<Urho3D::Vector3>{pos_ur_hit};
}


template <class TOp, typename... Args>
Op::Ptr UrSurface::enqueue(Args&&... args)
{
	Op::Ptr op = std::make_shared<TOp>(std::forward<Args>(args)...);
	Pause pause{this};
	m_queue_pending.push_back(op);
	return op;
}


void UrSurface::flush_physics_impl()
{
	if (m_polys.changes().size()) 
	{
		m_psurface_body->DisableMassUpdate();
		for (const Felt::PosIdx pos_idx_child : m_polys.changes())
		{
			const bool has_surface = m_surface.is_intersected(pos_idx_child);
			UrSurfaceCollisionShape * pshape = m_coll_shapes.get(pos_idx_child);

			// If the surface cuts through this partition and there isn't currently a collision 
			// shape covering it, create a new collition shape.
			if (has_surface)
			{
				// If there isn't currently a collision shape covering this partition.
				if (pshape == nullptr)
				{
					pshape = m_pnode->CreateComponent<UrSurfaceCollisionShape>();
					pshape->SetSurface(
						&m_surface.isogrid(), &m_surface.isogrid().children().get(pos_idx_child),
						&m_polys.children().get(pos_idx_child)
					);
					m_coll_shapes.set(pos_idx_child, pshape);
				}
				// Recalculate compound collision shape AABB tree.
				pshape->NotifyRigidBody(false);
			}	
			// If the surface no longer cuts through this partition, delete the collision shape.
			else if (pshape != nullptr)
			{
				m_pnode->RemoveComponent(pshape);
				m_coll_shapes.set(pos_idx_child, nullptr);
			}
		}
		m_psurface_body->EnableMassUpdate();
	}
}


void UrSurface::flush_graphics_impl()
{
	for (const Felt::PosIdx pos_idx_child : m_polys.changes())
	{
		m_gpu_polys.get(pos_idx_child).flush();
	}
}


void UrSurface::polygonise()
{
	m_polys.march();
}


void UrSurface::save(std::ostream& output_stream_) const
{
	m_surface.save(output_stream_);
}

UrFelt::Surface::Stats UrSurface::stats()
{
	return m_surface.stats();
}

UrSurface::UrSurface(Surface&& surface_, Urho3D::Node* pnode_) :
	m_exit{false},
	m_lock_pending{}, m_lock_done{},
	m_queue_pending{}, m_queue_done{},
	m_surface{std::move(surface_)},
	m_polys{m_surface},
	m_coll_shapes{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), nullptr
	},
	m_gpu_polys{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), GPUPoly{}
	},
	m_pnode{pnode_}
{
	for (
		Felt::PosIdx pos_idx_child = 0; pos_idx_child < m_polys.children().data().size();
		pos_idx_child++
	) {
		m_gpu_polys.get(pos_idx_child).bind(
			&m_polys.children().get(pos_idx_child), pnode_
		);
	}

    create_surface_body(pnode_);

	m_executor = std::thread{&UrSurface::executor, this};
}

void UrSurface::create_surface_body(Urho3D::Node* pnode_)
{
	m_psurface_body = pnode_->CreateComponent<Urho3D::RigidBody>();
	m_psurface_body->SetKinematic(true);
	m_psurface_body->SetMass(0);
	m_psurface_body->SetFriction(0.9f);
	m_psurface_body->SetUseGravity(false);
	m_psurface_body->SetRestitution(0.1);
}
} // UrFelt.


