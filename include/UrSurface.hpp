#ifndef INCLUDE_URSURFACE3D_HPP_
#define INCLUDE_URSURFACE3D_HPP_

#include <memory>
#include <type_traits>
#include <functional>
#include <queue>
#include <thread>
#include <atomic>
#include <future>

#include <boost/coroutine/all.hpp>
#include <boost/smart_ptr/detail/spinlock.hpp>

#include <sol.hpp>

#include <Felt/Impl/Tracked.hpp>
#include <Felt/Surface.hpp>

#include "GPUPoly.hpp"
#include "Op/Base.hpp"


namespace Urho3D
{
class RigidBody;
class Node;
}

namespace UrFelt
{

class UrSurfaceCollisionShape;


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

	static void to_lua(sol::table& lua);

	UrSurface () = delete;
	UrSurface(
		const Urho3D::IntVector3& size_, const Urho3D::IntVector3& size_partition_,
		Urho3D::Node* pnode_
	);
	UrSurface(
		const Felt::Vec3i& size_, const Felt::Vec3i& size_partition_,
		Urho3D::Node* pnode_
	);
	UrSurface(Surface&& surface_, Urho3D::Node* pnode_);

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
	* Wait for the executor to complete, then call callbacks and clear the queue.
	*/
	void await();

	/**
	* Poll completed ops and call callbacks.
	*/
	void poll();

	/**
	 * Cast ray to surface.
	 *
	 * @param ray_ ray to cast.
	 * @return position on surface that ray hits, or `ray_miss` if didn't hit.
	 */
	Urho3D::Vector3 ray(const Urho3D::Ray& ray_) const;


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
	 * March through changed spatial partitions, re-polygonising them.
	 */
	void polygonise();

	/**
	 * Save surface to disk.
	 *
	 * Actually just saves the isogrid, but that is sufficient to regenerate the surface.
	 *
	 * @see load
	 *
	 * @param file_path_ path to file on disk.
	 */
	void save(const std::string& file_path_) const;

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

	/**
	* Construct GPU assets and add to scene.
	*
	* NOTE: if used, then physics will become out of sync and `invalidate()` must be called to
	* before `flush` to recalculate and flush all physics assets.
	*/
	void flush_graphics();

private:
	/**
	 * Construct from (deserialised) `Felt::Surface`.
	 *
	 * @param surface_ rvalue surface to move into this.
	 */
	UrSurface(Surface&& surface_);

	/**
	* Enqueue an operation that should be processed in a worker thread.
	*
	* Ops can have an optional Lua callback function, which will be called in the main thread
	* on poll() or await() once the op is complete (or cancelled).
	*
	* @param op_ operation instance derived from Op::Base.
	*/
	template <class TOp, typename... Args>
	Op::Ptr enqueue(Args&&... args);

	/**
	 * Worker thread function that pops `Op`s from the queue and executes them.
	 */
	void executor();

	void flush_physics_impl();
	void flush_graphics_impl();

	static constexpr Felt::TupleIdx layer_idx(Felt::LayerId layer_id_)
	{
		return Surface::layer_idx(layer_id_);
	}

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
