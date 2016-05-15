/*
 * UrSurface.hpp
 *
 *  Created on: 13 Jul 2015
 *      Author: dave
 */

#ifndef INCLUDE_URSURFACE_HPP_
#define INCLUDE_URSURFACE_HPP_
#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>

#include <Felt/PolyGrid.hpp>
#include <Felt/Surface.hpp>

#include "UrPoly3D.hpp"

namespace felt
{

class UrPolyGrid3D;

template <> struct GridTraits<UrPolyGrid3D>
{
	using ThisType = UrPolyGrid3D;
	using LeafType = UrPoly3D;
	static const UINT Dims = 3;
};


class UrPolyGrid3D : public PolyGridBase<UrPolyGrid3D>
{
public:
	typedef PolyGridBase<UrPolyGrid3D>	Base;
	typedef Surface<3, 3>				Surface_t;

	typedef typename Base::VecDu	VecDu;
	typedef typename Base::VecDi	VecDi;

protected:
	Urho3D::Node*		m_pnode_root;
	Urho3D::Context* 	m_pcontext;

public:
	UrPolyGrid3D ();
	UrPolyGrid3D (
		const Surface_t& surface_, Urho3D::Context* pcontext_,
		Urho3D::Node* pnode_root_
	);
	~UrPolyGrid3D ();

	void init (
		const Surface_t& surface_, Urho3D::Context* pcontext_,
		Urho3D::Node* pnode_root_
	);

	void update_gpu ();

	void init_child (
		const VecDi& pos_child_, const VecDu& dims_, const VecDi& offset_
	);
};




}
#endif /* INCLUDE_URSURFACE_HPP_ */

