#ifndef INCLUDE_URSURFACE_HPP_
#define INCLUDE_URSURFACE_HPP_
#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>

#include <Felt/Polys.hpp>
#include <Felt/Surface.hpp>
#include <Felt/Polys.hpp>

#include "UrPoly3D.hpp"

namespace Felt
{

class UrPolyGrid3D : public Polys<typename Traits::SurfaceType>
{
private:
	using ThisType = UrPolyGrid3D;
	using Base = PolyGridBase<ThisType>;
	using SurfaceType = typename Traits<ThisType>::SurfaceType;
	using VecDi = Felt::VecDi<3>;

	Urho3D::Node*		m_pnode_root;
	Urho3D::Context* 	m_pcontext;

public:
	UrPolyGrid3D ();
	UrPolyGrid3D (
		const SurfaceType& surface_, Urho3D::Context* pcontext_,
		Urho3D::Node* pnode_root_
	);
	~UrPolyGrid3D ();

	void update_gpu ();
};



namespace Impl
{
/**
 * Traits for UrPolyGrid3D.
 */
template <>
struct Traits< UrPolyGrid3D >
{
	/// Type of surface to polygonise.
	using SurfaceType = Surface<3, 3>;
	/// Isogrid type that the surface wraps.
	using IsoGridType = typename SurfaceType::IsoGrid;
	/// Dimension of grid.
	static constexpr Dim t_dims = Traits<IsoGridType>::t_dims;
	/// Child poly type to polygonise a single spatial partition.
	using ChildType = UrPoly3D;
	/// Children grid type to store and track active child polys.
	using ChildrenType = Impl::Tracked::SingleListSingleIdxByRef<ChildType, t_dims>;
};
} // Impl.

}
#endif /* INCLUDE_URSURFACE_HPP_ */

