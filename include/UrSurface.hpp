#ifndef INCLUDE_URSURFACE3D_HPP_
#define INCLUDE_URSURFACE3D_HPP_

#include <functional>
#include <queue>
#include <thread>
#include <atomic>
#include <future>

#include <boost/coroutine/all.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

#include <Urho3D/ThirdParty/toluapp/tolua++.h>
#include <sol.hpp>
#define cimg_display 0
#include <CImg.h>

#include <Felt/Impl/Tracked.hpp>
#include <Felt/Surface.hpp>
#include "GPUPoly.hpp"


namespace Urho3D
{
class RigidBody;
class Node;
}

namespace UrFelt
{

class UrSurfaceCollisionShape;

class UrSurfaceOp;

class UrSurface
{
private:
	using Surface = Felt::Surface<3, 3>;
	using Polys = Felt::Polys<Surface>;
	using CollShapes = Felt::Impl::Tracked::SingleListSingleIdxByValue<UrSurfaceCollisionShape*, 3>;
	using GPUPolys = Felt::Impl::Tracked::SingleListSingleIdxByRef<GPUPoly, 3>;

	using Lock = boost::detail::spinlock;
	using Guard = std::lock_guard<Lock>;

public:
	using IsoGrid = Surface::IsoGrid;
	static const Felt::Vec3f ray_miss;
private:
//	using UpdateFn = std::function<Felt::Distance(const Felt::Vec3i&, const IsoGrid&)>;
public:

	struct Op
	{
		#define URSURFACE_OP_CLONE(Type)\
			std::unique_ptr<Base> clone() const { return std::unique_ptr<Base>(new Type(*this)); };

		struct Base
		{
			sol::function callback;
			virtual void execute(UrSurface& surface) = 0;
			virtual std::unique_ptr<Base> clone() const = 0;
			virtual bool is_complete();
		protected:
			Base() = default;
			Base(sol::function callback_);
		};

		using Ptr = std::unique_ptr<UrSurface::Op::Base>;

		struct Polygonise : Base
		{
			Polygonise() = default;
			Polygonise(sol::function callback_);
			void execute(UrSurface& surface);
			URSURFACE_OP_CLONE(Polygonise)
		};

		struct ExpandByConstant : Base
		{
			ExpandByConstant(const float amount_);
			ExpandByConstant(const float amount_, sol::function callback_);
			void execute(UrSurface& surface);
			bool is_complete();
			URSURFACE_OP_CLONE(ExpandByConstant)
		private:
			float m_amount;
		};

		struct ExpandToBox : Base
		{
			ExpandToBox(const Urho3D::Vector3& pos_start_, const Urho3D::Vector3& pos_end_);
			ExpandToBox(
				const Urho3D::Vector3& pos_start_, const Urho3D::Vector3& pos_end_,
				sol::function callback_
			);
			void execute(UrSurface& surface);
			bool is_complete();
			URSURFACE_OP_CLONE(ExpandToBox)
		private:
			bool m_is_complete;
			const Felt::Vec3f	m_pos_min;
			const Felt::Vec3f	m_pos_max;
			const Felt::Vec3f	m_pos_centre;
			Felt::Vec3f	m_pos_COM;
			Felt::ListIdx	m_size;
			std::vector<Surface::Plane>	m_planes;

		};

		struct ExpandToImage : Base
		{
			ExpandToImage(
				const std::string& file_name_, const float ideal_, const float tolerance_,
				const float curvature_weight_
			);
			ExpandToImage(
				const std::string& file_name_, const float ideal_, const float tolerance_,
		const float curvature_weight_, sol::function callback_
			);
			void execute(UrSurface& surface);
			bool is_complete();
			URSURFACE_OP_CLONE(ExpandToImage)
		private:
			bool m_is_complete;
			const std::string m_file_name;
			const float m_ideal;
			const float m_tolerance;
			const float m_curvature_weight;
			cimg_library::CImg<Felt::Distance>	m_image;
		};
	};

	static void to_lua(sol::table& lua);

	UrSurface () = default;
	UrSurface(
		const Urho3D::IntVector3& size_, const Urho3D::IntVector3& size_partition_,
		Urho3D::Node* pnode_
	);
	UrSurface(
		const Felt::Vec3i& size_, const Felt::Vec3i& size_partition_,
		Urho3D::Node* pnode_
	);

	~UrSurface();

	/**
	* Perform a a full (parallelised) update of the narrow band.
	*
	* Lambda function passed will be given the position to process and
	* a reference to the phi grid, and is expected to return delta phi to
	* apply.
	*
	* @param fn_ (pos, phi) -> float
	*/
	template <typename Fn>
	void update(Fn&& fn_)
	{
		m_surface.update(fn_);
		m_polys.notify();
	}

	/**
	* Perform a a full (parallelised) update of the narrow band.
	*
	* Lambda function passed will be given the position to process and
	* a reference to the phi grid, and is expected to return delta phi to
	* apply.
	*
	* @param fn_ (pos, phi) -> float
	*/
	template <typename TFn>
	void update(
		const Felt::Vec3i& pos_leaf_lower_, const Felt::Vec3i& pos_leaf_upper_,
		TFn&& fn_
	) {
		m_surface.update(pos_leaf_lower_, pos_leaf_upper_, fn_);
		m_polys.notify();
	}

	/**
	* Loop changed spatial partitions and construct polygonisation.
	*/
	void polygonise();

	/**
	* Enqueue a full (parallelised) update of the narrow band.
	*
	* Lambda function passed will be given the position to process and
	* a reference to the phi grid, and is expected to return delta phi to
	* apply.
	*
	* @param fn_ (pos, phi) -> float
	*/
	void enqueue(const UrSurface::Op::Base& op_);

	/**
	* Wait for the executor to complete, then call callbacks and clear the queue.
	*/
	void await();

	/**
	* Poll completed ops and call callbacks.
	*/
	void poll();


	Felt::Vec3f ray(const Urho3D::Vector3& pos_origin_, const Urho3D::Vector3& dir_) const
	{
		return m_surface.ray(
			reinterpret_cast<const Felt::Vec3f&>(pos_origin_),
			reinterpret_cast<const Felt::Vec3f&>(dir_)
		);
	}

	static constexpr Felt::TupleIdx layer_idx(Felt::LayerId layer_id_)
	{
		return Surface::layer_idx(layer_id_);
	}

	const IsoGrid& isogrid() const
	{
		return m_surface.isogrid();
	}

	/**
	* Create singularity seed surface (single zero-layer point).
	*
	* @param pos_centre_ position of seed.
	*/
	void seed (const Urho3D::IntVector3& pos_centre_)
	{
		m_surface.seed(reinterpret_cast<const Felt::Vec3i&>(pos_centre_));
	}

	/**
	* Create singularity seed surface (single zero-layer point).
	*
	* @param pos_centre_ position of seed.
	*/
	void invalidate ()
	{
		m_polys.invalidate();
	}

	/**
	* Construct physics and GPU assets and add to scene.
	*/
	void flush();

private:
	void executor();

	std::atomic_bool m_pause;
	std::atomic_bool m_exit;

	std::thread m_executor;

	Lock	m_lock_pending;
	Lock	m_lock_done;

	std::deque<Op::Ptr>	m_queue_pending;
	std::deque<Op::Ptr>	m_queue_done;

	UrFelt::Surface		m_surface;
	UrFelt::Polys		m_polys;
	CollShapes			m_coll_shapes;
	GPUPolys			m_gpu_polys;
	Urho3D::Node* 		m_pnode;
	Urho3D::RigidBody* 	m_psurface_body;


	struct Pause
	{
		Pause(UrSurface* self) :
			m_self{self}
		{
			m_self->m_pause = true;
			m_self->m_lock_pending.lock();
		}
		~Pause()
		{
			m_self->m_pause = false;
			m_self->m_lock_pending.unlock();
		}
		UrSurface* m_self;
	};

	friend struct Pause;
};

} /* namespace UrFelt */

#endif /* INCLUDE_URSURFACE3D_HPP_ */
