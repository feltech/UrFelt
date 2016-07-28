/*
 * UrSurface3D.hpp
 *
 *  Created on: 28 Jul 2015
 *      Author: dave
 */

#ifndef INCLUDE_URSURFACE3D_HPP_
#define INCLUDE_URSURFACE3D_HPP_

#include <functional>
#include <boost/coroutine/all.hpp>

#include <Felt/Surface.hpp>
#include "UrPolyGrid3D.hpp"

namespace felt
{


class UrSurface3D : public Surface<3, 3>
{
public:
	using Base = Surface<3, 3>;
	using Base::VecDu;
	using Base::VecDi;
	using Base::VecDf;
protected:
	UrPolyGrid3D	m_poly;
	Urho3D::Node* 	m_pnode;

	UINT	m_physics_init;

public:
	UrSurface3D () = default;


	UrSurface3D(
		Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_,
		const VecDu& dims_,
		const VecDu& dims_partition_ = VecDu::Constant(8)
	);

	~UrSurface3D();

	void init(
		Urho3D::Context* pcontext_, Urho3D::Node* pnode_root_,
		const VecDu& dims_,
		const VecDu& dims_partition_ = VecDu::Constant(8)
	);

	FLOAT init_physics_chunk();

	const UrPolyGrid3D& poly() const;
	UrPolyGrid3D& poly();


	/**
	 * Perform a a full (parallelised) update of the narrow band.
	 *
	 * Lambda function passed will be given the position to process and
	 * a reference to the phi grid, and is expected to return delta phi to
	 * apply.
	 *
	 * @param fn_ (pos, phi) -> float
	 */
	void update(std::function<FLOAT(const VecDi&, const IsoGrid&)> fn_);

protected:
	boost::coroutines::coroutine<FLOAT>::pull_type m_initWatcher;

	void init_physics_task(boost::coroutines::coroutine<FLOAT>::push_type& sink);
};

} /* namespace felt */

#endif /* INCLUDE_URSURFACE3D_HPP_ */
