#ifndef INCLUDE_URSURFACE3D_HPP_
#define INCLUDE_URSURFACE3D_HPP_

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

public:
	using IsoGrid = Surface::IsoGrid;
	static const Felt::Vec3f ray_miss;
private:
//	using UpdateFn = std::function<Felt::Distance(const Felt::Vec3i&, const IsoGrid&)>;
public:

	struct Op
	{
		#define URSURFACE_OP_CLONE(Type)\
			std::unique_ptr<Base> clone() { return std::unique_ptr<Base>(new Type(*this)); };

		struct Base
		{
			sol::function callback;
			virtual void execute(UrSurface& surface) = 0;
			virtual std::unique_ptr<Base> clone() = 0;
		protected:
			Base(sol::function callback_);
		};

		struct Polygonise : Base
		{
			Polygonise(sol::function callback_);
			void execute(UrSurface& surface);
			URSURFACE_OP_CLONE(Polygonise)
		};

		struct Simple : Base
		{
			Simple(const float amount_, sol::function callback_);
			void execute(UrSurface& surface);
			URSURFACE_OP_CLONE(Simple)
		private:
			const float m_amount;
		};
	};

	static void to_lua(sol::table& lua);

	UrSurface () = default;

	UrSurface(
		const Felt::Vec3i& size_,
		const Felt::Vec3i& size_partition_,
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
	void enqueue(UrSurface::Op::Base* op);

	/**
	 * Execute enqueued updates.
	 */
	void wake();

	/**
	 * Wait for the executor to complete, then call callbacks and clear the queue.
	 */
	void await();


	Felt::Vec3f ray(const Felt::Vec3f& pos_origin_, const Felt::Vec3f& dir_) const
	{
		return m_surface.ray(pos_origin_, dir_);
	}

	/**
	 * Create singularity seed surface (single zero-layer point).
	 *
	 * @param pos_centre_ position of seed.
	 */
	void seed (const Felt::Vec3i& pos_centre_)
	{
		m_surface.seed(pos_centre_);
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

	bool m_exit;
	std::mutex m_mutex_executor;
	std::condition_variable m_execution;
	std::thread m_executor;

	std::vector<std::unique_ptr<UrSurface::Op::Base>>	m_queue_executor;

	UrFelt::Surface		m_surface;
	UrFelt::Polys		m_polys;
	CollShapes			m_coll_shapes;
	GPUPolys			m_gpu_polys;
	Urho3D::Node* 		m_pnode;
	Urho3D::RigidBody* 	m_psurface_body;

};

} /* namespace UrFelt */

#endif /* INCLUDE_URSURFACE3D_HPP_ */
