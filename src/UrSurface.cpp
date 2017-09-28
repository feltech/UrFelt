#include "UrSurface.hpp"

#include <vector>
#include <algorithm>
#include <limits>

#include <Urho3D/Physics/RigidBody.h>
#include <Felt/Impl/Util.hpp>

#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"


namespace sol {
namespace stack {
	template <typename T>
	struct userdata_checker<extensible<T>> {
		template <typename Handler>
		static bool check(
			lua_State* L, int relindex, type index_type, Handler&& handler, record& tracking
		) {
			// just marking unused parameters for no compiler warnings
			(void)index_type;
			(void)handler;
			int index = lua_absindex(L, relindex);
			std::string name = sol::detail::short_demangle<T>();
			tolua_Error tolua_err;
			return tolua_isusertype(L, index, name.c_str(), 0, &tolua_err);
		}
	};
}
} // namespace sol::stack



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

	lua_Op.new_usertype<UrSurface::Op::ExpandToBox>(
		"ExpandToBox",
		sol::call_constructor,
		sol::constructors<
			UrSurface::Op::ExpandToBox(
				const Urho3D::Vector3&, const Urho3D::Vector3&, sol::function
			),
			UrSurface::Op::ExpandToBox(const Urho3D::Vector3&, const Urho3D::Vector3&)
		>()
	);
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
	m_surface{size_, size_partition_},
	m_coll_shapes{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), nullptr
	},
	m_polys{m_surface},
	m_gpu_polys{
		m_surface.isogrid().children().size(), m_surface.isogrid().children().offset(), GPUPoly{}
	},
	m_pnode{pnode_}, m_exit{false},
	m_queue_pending{}, m_queue_done{}, m_lock_pending{}, m_lock_done{}
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
	Pause pause{this};
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
	m_pause = false;
}


void UrSurface::enqueue(const UrSurface::Op::Base& op_)
{
	Pause pause{this};
	m_queue_pending.push_back(op_.clone());
	m_pause = false;
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
			Guard lock(m_lock_pending);
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


UrSurface::Op::ExpandToBox::ExpandToBox(
	const Urho3D::Vector3& pos_start_, const Urho3D::Vector3& pos_end_, sol::function callback_
) :
	UrSurface::Op::Base{callback_},
	m_pos_start{reinterpret_cast<const Felt::Vec3f&>(pos_start_)},
	m_pos_end{reinterpret_cast<const Felt::Vec3f&>(pos_end_)},
	m_is_complete{false}
{
	volatile int i = 0;
}


UrSurface::Op::ExpandToBox::ExpandToBox(
	const Urho3D::Vector3& pos_start_, const Urho3D::Vector3& pos_end_
) :
	ExpandToBox{pos_start_, pos_end_, sol::function{}}
{}


void UrSurface::Op::ExpandToBox::execute(UrSurface& surface)
{
	m_is_complete = true;

	surface.update(
		[this](const Felt::Vec3i& pos_, const IsoGrid& isogrid_) {
			using namespace Felt;

			const Vec3f& grad = isogrid_.gradE(pos_);
			if (grad.isZero())
				return 1.0f;

			const Vec3f& normal = grad.normalized();
			const Felt::Distance mag_grad = grad.norm();
			const Felt::Distance dist_surf = isogrid_.get(pos_);
			const Vec3f& fpos = pos_.template cast<Felt::Distance>() - normal*dist_surf;
			const bool inside = Felt::inside(fpos, m_pos_start, m_pos_end);
			const float orientation = (2*float(inside) - 1);


			std::vector<Surface::Plane> planes;
			for (Felt::Dim d = 0; d < m_pos_start.size(); d++)
			{
				Vec3f plane_normal = Felt::Vec3f::Zero();
				plane_normal(d) = -1;
				Surface::Plane plane{plane_normal, m_pos_start(d)};
				planes.push_back(plane);
			}
			for (Felt::Dim d = 0; d < m_pos_end.size(); d++)
			{
				Vec3f plane_normal = Felt::Vec3f::Zero();
				plane_normal(d) = 1;
				Surface::Plane plane{plane_normal, -m_pos_end(d)};
				planes.push_back(plane);
			}
			Vec3f pos_intersect = Vec3f::Constant(std::numeric_limits<Distance>::max());
			const Vec3f& pos_box_centre = (m_pos_end - m_pos_start).template cast<Distance>() /
				2 + m_pos_start.template cast<Distance>();

			Surface::Line line{pos_box_centre, normal};

			for (const Surface::Plane& plane : planes)
			{
				const Vec3f pos_test = line.intersectionPoint(plane);
				if (pos_test.dot(normal) > 0)
					if (
						(pos_test - pos_box_centre).squaredNorm() <
						(pos_intersect - pos_box_centre).squaredNorm()
					) {
						pos_intersect = pos_test;
					}
			}

			const Vec3f& displacement_from_ideal =  pos_intersect - fpos;

			Distance force_speed;
			if (displacement_from_ideal.squaredNorm() > 1.0f)
				force_speed = normal.dot(displacement_from_ideal.normalized());
			else
				force_speed = normal.dot(displacement_from_ideal);

			const Felt::Distance amount = -force_speed*0.5f + 0.1f * isogrid_.curv(fpos);

			m_is_complete &= std::abs(amount) <= std::numeric_limits<float>::epsilon();

			const Felt::Distance amount_clamped =
				std::min(std::max(amount, -1.0f), 1.0f);

			return amount_clamped;
		}
	);
}


bool UrSurface::Op::ExpandToBox::is_complete()
{
	return m_is_complete;
}

} // UrFelt.

