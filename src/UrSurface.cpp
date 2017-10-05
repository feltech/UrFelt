#include "UrSurface.hpp"

#include <vector>
#include <algorithm>
#include <limits>

#include <Urho3D/Physics/RigidBody.h>
#include <Felt/Impl/Util.hpp>

#include "btFeltCollisionConfiguration.hpp"
#include "UrSurfaceCollisionShape.hpp"


namespace sol
{
	namespace stack
	{
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
		"polygonise", &UrSurface::polygonise,
		"flush", &UrSurface::flush,
		"flush_graphics", &UrSurface::flush_graphics,

		"update", [](UrSurface& self, sol::function fn_) {
			self.update([&fn_](const Felt::Vec3i& pos_, const UrSurface::IsoGrid& isogrid_) {
				Urho3D::IntVector3 vpos_ = reinterpret_cast<const Urho3D::IntVector3&>(pos_);
				Felt::Distance dist = fn_(vpos_, isogrid_);
				return dist;
			});
		},

		"enqueue", sol::overload(
			&UrSurface::enqueue<UrSurface::Op::Polygonise>,
			&UrSurface::enqueue<UrSurface::Op::ExpandByConstant>,
			&UrSurface::enqueue<UrSurface::Op::ExpandToBox>,
			&UrSurface::enqueue<UrSurface::Op::ExpandToImage>
		),
		"await", &UrSurface::await,
		"poll", &UrSurface::poll
	);

	sol::table lua_Op = lua.create_named("Op");

	lua_Op.new_usertype<UrSurface::Op::Polygonise>(
		"Polygonise",
		sol::call_constructor,
		sol::factories(
			&UrSurface::Op::Polygonise::factory<>,
			&UrSurface::Op::Polygonise::factory<sol::function>
		),
		"stop", &UrSurface::Op::Polygonise::stop
	);

	lua_Op.new_usertype<UrSurface::Op::ExpandByConstant>(
		"ExpandByConstant",
		sol::call_constructor,
		sol::factories(
			&UrSurface::Op::ExpandByConstant::factory<float>,
			&UrSurface::Op::ExpandByConstant::factory<float, sol::function>
		),
		"stop", &UrSurface::Op::ExpandByConstant::stop
	);

	lua_Op.new_usertype<UrSurface::Op::ExpandToBox>(
		"ExpandToBox",
		sol::call_constructor,
		sol::factories(
			&UrSurface::Op::ExpandToBox::factory<
				const Urho3D::Vector3&, const Urho3D::Vector3&, sol::function
			>,
			&UrSurface::Op::ExpandToBox::factory<const Urho3D::Vector3&, const Urho3D::Vector3&>
		),
		"stop", &UrSurface::Op::ExpandToBox::stop
	);

	lua_Op.new_usertype<UrSurface::Op::ExpandToImage>(
		"ExpandToImage",
		sol::call_constructor,
		sol::factories(
			&UrSurface::Op::ExpandToImage::factory<
				const std::string&, const float, const float, const float, sol::function
			>,
			&UrSurface::Op::ExpandToImage::factory<
				const std::string&, const float, const float, const float
			>
		),
		"stop", &UrSurface::Op::ExpandToImage::stop
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
	flush_physics_impl();
	flush_graphics_impl();
}


void UrSurface::flush_graphics()
{
	Pause pause{this};
	flush_graphics_impl();
}


void UrSurface::flush_physics_impl()
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
	}
}


void UrSurface::flush_graphics_impl()
{
	for (const Felt::PosIdx pos_idx_child : m_polys.changes())
	{
		m_gpu_polys.get(pos_idx_child).flush();
	}
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
	std::deque<Op::BasePtr> queue_done;
	{
		Guard lock(m_lock_done);
		queue_done = std::move(m_queue_done);
	}

	for (Op::BasePtr& op : queue_done)
		if (op->callback)
			op->callback();
}


UrSurface::Op::Base::Base(sol::function callback_) :
	callback{callback_}, m_cancelled{false}
{}


bool UrSurface::Op::Base::is_complete()
{
	return true;
}

void UrSurface::Op::Base::stop()
{
	m_cancelled = true;
}

UrSurface::Op::Polygonise::Polygonise(sol::function callback_) :
	UrSurface::Op::Base{callback_}
{}


void UrSurface::Op::Polygonise::execute(UrSurface& surface)
{
	if (this->m_cancelled)
		return;

	surface.polygonise();
}


UrSurface::Op::ExpandByConstant::ExpandByConstant(const float amount_) :
	UrSurface::Op::ExpandByConstant{amount_, sol::function{}}
{}


UrSurface::Op::ExpandByConstant::ExpandByConstant(const float amount_, sol::function callback_) :
	UrSurface::Op::Base{callback_}, m_amount{amount_}
{}


void UrSurface::Op::ExpandByConstant::execute(UrSurface& surface)
{
	if (this->m_cancelled)
		return;

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
	m_pos_min{reinterpret_cast<const Felt::Vec3f&>(pos_start_)},
	m_pos_max{reinterpret_cast<const Felt::Vec3f&>(pos_end_)},
	m_is_complete{false},
	m_size{0},
	m_pos_centre{
		m_pos_min.template cast<Felt::Distance>() +
		(m_pos_max - m_pos_min).template cast<Felt::Distance>() / 2
	}
{
	for (Felt::Dim d = 0; d < m_pos_min.size(); d++)
	{
		Felt::Vec3f plane_normal = Felt::Vec3f::Zero();
		plane_normal(d) = -1;
		Surface::Plane plane{plane_normal, m_pos_min(d)};
		m_planes.push_back(plane);
	}
	for (Felt::Dim d = 0; d < m_pos_max.size(); d++)
	{
		Felt::Vec3f plane_normal = Felt::Vec3f::Zero();
		plane_normal(d) = 1;
		Surface::Plane plane{plane_normal, -m_pos_max(d)};
		m_planes.push_back(plane);
	}
}


UrSurface::Op::ExpandToBox::ExpandToBox(
	const Urho3D::Vector3& pos_start_, const Urho3D::Vector3& pos_end_
) :
	ExpandToBox{pos_start_, pos_end_, sol::function{}}
{}


void UrSurface::Op::ExpandToBox::execute(UrSurface& surface)
{
	// Assume complete until at least one point has a distance update greater than epsilon.
	m_is_complete = true;
	if (this->m_cancelled)
		return;
	// Take a copy of surface size and centre of mass, before resetting it.
	const Felt::Distance size = m_size;
	Felt::Vec3f pos_COM = m_pos_COM;
	// Reset surface size (number of zero layer points) and surface's centre of mass to zero,
	// for incremental recalculation below.
	m_size = 0;
	m_pos_COM = Felt::Vec3f{0,0,0};

	surface.update(
		[this, &pos_COM, &size](const Felt::Vec3i& pos_, const IsoGrid& isogrid_) {
			using namespace Felt;
			// Size of surface update to consider zero (thus finished).
			static constexpr Distance epsilon = 0.0001;
			// Multiplier to ensure no surface update with magnitude greater than 1.0.
			// TODO: understand why grad can be > 2 and if sqrt(2) per component theory is correct.
			static constexpr Distance clamp = 1.0f/sqrt(6.0f);

			// Get entropy-satisfying gradient (surface "normal").
			const Vec3f& grad = isogrid_.gradE(pos_);
			// If the gradient is zero, we must have a singularity, so just trivially contract
			// (i.e. destroy).
			if (grad.isZero())
				return 1.0f;
			// Get distance of this discrete zero-layer point to the continuous zero-level set
			// surface.
			const Felt::Distance dist_surf = isogrid_.get(pos_);

			// Discretisation means grad can be non-normalised, so normalise it.
			const Vec3f& normal = grad.normalized();
			// Interpolate discrete zero-layer grid point to continuous zero-level isosurface.
			const Vec3f& fpos = pos_.template cast<Felt::Distance>() - normal*dist_surf;
			// Get euclidean norm of the gradient, for use in level set update equations.
			const Distance grad_norm = grad.norm();

			// Incremental update of centre of mass.
			m_size++;
			m_pos_COM += fpos;

			// Direction from centre of mass of surface to this surface point.
			Vec3f dir_from_COM;

			if (size == 0)
			{
				// If size (thus centre of mass) has not yet been calculated (i.e. this is the
				// first iteration) just assume the line from the centre of mass is the same as the
				// surface normal.
				dir_from_COM = normal;
			}
			else
			{
				// Displacement of surface point from surface's centre of mass.
				const Vec3f& disp_from_COM = fpos - pos_COM;
				// If the surface is tiny, don't allow it to collapse.
				if (disp_from_COM.squaredNorm() <= 1.0f/clamp)
				{
					m_is_complete = false;
					return -grad_norm*clamp;
				}
				// Get direction of surface point from surface's centre of mass.
				dir_from_COM = disp_from_COM.normalized();
			}

			// Get ray from centre of target box along direction given by surface centre of mass
			// to surface point.
			const Surface::Line& line{m_pos_centre, dir_from_COM};

			// Calculate point on box where this surface point wants to head toward.
			Vec3f pos_target = Vec3f::Constant(std::numeric_limits<Distance>::max());
			for (const Surface::Plane& plane : m_planes)
			{
				// Find intersection point of closest plane enclosing target box and ray from centre
				// of mass of target box that matches ray from centre of mass of surface to surface
				// point.
				const Vec3f& pos_test = line.intersectionPoint(plane);
				if (plane.normal().dot(dir_from_COM) > 0)
					if (
						(pos_test - m_pos_centre).squaredNorm() <
						(pos_target - m_pos_centre).squaredNorm()
					) {
						pos_target = pos_test;
					}
			}

			// Get displacement of surface point to it's target point on the box.
			const Vec3f& displacement_to_target =  pos_target - fpos;

			// Calculate "impulse" to apply to surface point along it's normal to get it heading
			// toward target box point.
			Distance impulse;
			if (displacement_to_target.squaredNorm() > 1.0f)
				// If we're far from the target point, clamp the impulse to +/-1.
				impulse = normal.dot(displacement_to_target.normalized());
			else
				// If we're near the target point, the impulse is directly proportional to the
				// distance.
				impulse = normal.dot(displacement_to_target);

			// The level set update.
			const Felt::Distance amount = -clamp*grad_norm*impulse;

			// Flag that the operation is incomplete if this surface point wants to move a distance
			// larger than epsilon.
			m_is_complete &= std::abs(amount) <= epsilon;

			return amount;
		}
	);

	m_pos_COM /= m_size;
}


bool UrSurface::Op::ExpandToBox::is_complete()
{
	return m_is_complete;
}


UrSurface::Op::ExpandToImage::ExpandToImage(
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_
) : UrSurface::Op::ExpandToImage{file_name_, ideal_, tolerance_, curvature_weight_, sol::function{}}
{}


UrSurface::Op::ExpandToImage::ExpandToImage(
	const std::string& file_name_, const float ideal_, const float tolerance_,
	const float curvature_weight_, sol::function callback_
) :
	UrSurface::Op::Base{callback_},
	m_is_complete{false}, m_file_name{file_name_}, m_ideal{ideal_}, m_tolerance{tolerance_},
	m_curvature_weight{curvature_weight_}, m_image{}, m_divisor{0}
{}


void UrSurface::Op::ExpandToImage::execute(UrSurface& surface)
{
	m_is_complete = true;

	if (this->m_cancelled)
		return;

	if (!m_image.size())
	{
		m_image.load(m_file_name.c_str());
		m_divisor = m_image.max();
		std::cerr << "Loaded " << m_file_name << " with " << m_image.size() << " pixels in a " <<
			m_image.width() << "x" <<  m_image.height() << "x" <<  m_image.depth() <<
			" configuration with max voxel value " << m_divisor << std::endl;
	}

	surface.update([this](const Felt::Vec3i& pos_, const IsoGrid& isogrid_) {
		using namespace Felt;
		// Size of surface update to consider zero (thus finished).
		static constexpr Distance epsilon = 0.0001;
		// Multiplier to ensure no surface update with magnitude greater than 1.0.
		static constexpr Distance clamp = 1.0f/(2*sqrt(6.0f));

		// Get entropy-satisfying gradient (surface "normal").
		const Vec3f& grad = isogrid_.gradE(pos_);
		// If the gradient is zero, we must have a singularity, so just trivially contract
		// (i.e. destroy).
		if (grad.isZero())
			return 1.0f;
		// Get distance of this discrete zero-layer point to the continuous zero-level set
		// surface.
		const Felt::Distance dist_surf = isogrid_.get(pos_);

		// Discretisation means grad can be non-normalised, so normalise it.
		const Vec3f& normal = grad.normalized();
		// Interpolate discrete zero-layer grid point to continuous zero-level isosurface.
		const Vec3f& fpos = pos_.template cast<Felt::Distance>() - normal*dist_surf;
		// Get euclidean norm of the gradient, for use in level set update equations.
		const Distance grad_norm = grad.norm();

		const Distance voxel = m_image.atXYZ(
			pos_(0) + m_image.width()/2, pos_(1) + m_image.height()/2, pos_(2) + m_image.depth()/2,
			0, std::numeric_limits<unsigned char>::max()
		)/m_divisor;

		const Distance speed =
			2*pow(voxel - m_ideal, 2) / ( pow(voxel - m_ideal, 2) + pow(m_tolerance, 2) ) - 1;
		const Distance curvature = isogrid_.curv(pos_);

		const Distance amount = grad_norm*(speed + m_curvature_weight*curvature)*clamp;

		m_is_complete &= std::abs(amount) <= epsilon;

		return amount;
	});
}


bool UrSurface::Op::ExpandToImage::is_complete()
{
	return m_is_complete;
}

} // UrFelt.

