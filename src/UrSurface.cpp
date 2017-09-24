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
	Guard lock{m_lock_pending};
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

			std::vector<Surface::Plane> planes_in;
			std::vector<Surface::Plane> planes_out;

			for (const Surface::Plane& plane : planes)
				if (normal.dot(plane.normal()) > 0)
					planes_in.push_back(plane);
				else
					planes_out.push_back(plane);

			std::sort(planes_in.begin(), planes_in.end(),
				[&pos_, &normal](const Surface::Plane& a, const Surface::Plane& b){
					const Vec3f& fpos = pos_.template cast<Felt::Distance>();
					const Surface::Line& line{fpos, normal};
					const Vec3f& pos_intersect_a = line.intersectionPoint(a);
					const Vec3f& pos_dist_a = fpos - pos_intersect_a;
					const Vec3f& pos_intersect_b = line.intersectionPoint(b);
					const Vec3f& pos_dist_b = fpos - pos_intersect_a;
					return pos_dist_a.squaredNorm() < pos_dist_b.squaredNorm();
				}
			);

			std::sort(planes_out.begin(), planes_out.end(),
				[&pos_, &normal](const Surface::Plane& a, const Surface::Plane& b){
					const Vec3f& fpos = pos_.template cast<Felt::Distance>();
					const Surface::Line& line{fpos, normal};
					const Vec3f& pos_intersect_a = line.intersectionPoint(a);
					const Vec3f& pos_dist_a = fpos - pos_intersect_a;
					const Vec3f& pos_intersect_b = line.intersectionPoint(b);
					const Vec3f& pos_dist_b = fpos - pos_intersect_a;
					return pos_dist_a.squaredNorm() < pos_dist_b.squaredNorm();
				}
			);

			const Vec3f& fpos = pos_.template cast<Felt::Distance>() - normal*dist_surf;
			Surface::Line line{fpos, normal};
			if (planes_in.size() == 0)
				volatile int i = 0;
			const Vec3f& pos_intersect = line.intersectionPoint(planes_in[0]);
			const Vec3f& pos_dist = pos_intersect - fpos;
			const Felt::Distance dist_side = normal.dot(pos_dist);

			const Felt::Distance dist_side_clamped =
				std::min(std::max(dist_side, -0.5f), 0.5f);

			const Felt::Distance amount = -mag_grad * dist_side_clamped;

			if (std::abs(pos_(1)) == 6 && std::abs(amount) < 0)
				volatile int i =0;

			m_is_complete &= std::abs(amount) <= std::numeric_limits<float>::epsilon();

			if (pos_(0) > m_pos_end(0) || pos_(1) > m_pos_end(1) || pos_(2) > m_pos_end(2))
				volatile int i=0;

			return amount;
		}
	);
}


bool UrSurface::Op::ExpandToBox::is_complete()
{
	return m_is_complete;
}

} // UrFelt.

