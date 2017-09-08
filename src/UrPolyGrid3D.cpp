#include "UrPolyGrid3D.hpp"

namespace Felt
{

UrPolyGrid3D::UrPolyGrid3D ()  : Base(), m_pnode_root(NULL), m_pcontext(NULL)
{}


UrPolyGrid3D::UrPolyGrid3D (
	const SurfaceType& surface_, Urho3D::Context* pcontext_,
	Urho3D::Node* pnode_root_
) : Base(surface_), m_pnode_root(pnode_root_), m_pcontext(pcontext_)
{
	// Initial.
	for (
		PosIdx pos_idx_child = 0; pos_idx_child < this->children().data().size();
		pos_idx_child++
	) {
		this->children().get(pos_idx_child).bind(
			surface_.isogrid().children().get(pos_idx_child).lookup()
		);
	}
}


UrPolyGrid3D::~UrPolyGrid3D ()
{}


void UrPolyGrid3D::init_child (
	const VecDi& pos_child_, const VecDu& dims_, const VecDi& offset_
) {
	Base::init_child(pos_child_, dims_, offset_);
	// Add a one-element border to account for partition overlap.
	this->get(pos_child_).init_gpu(dims_, offset_, m_pcontext, m_pnode_root);
}


void UrPolyGrid3D::update_gpu ()
{
	for (const Vec3i& pos_child : this->m_grid_changes.list())
		this->get(pos_child).update_gpu();
}

} /* namespace Felt */
