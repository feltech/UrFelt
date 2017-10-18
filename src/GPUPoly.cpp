#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/Log.h>

#include <Felt/Polys.hpp>
#include "GPUPoly.hpp"

namespace UrFelt
{

GPUPoly::~GPUPoly ()
{
	if (m_pvb.NotNull())
		m_pvb->Release();
	if (m_pib.NotNull())
		m_pib->Release();
//	if (m_pnode != nullptr)
//		m_pnode->Remove();
}

void GPUPoly::bind (
	const UrFelt::Polys::Child *const ppoly_, Urho3D::Node *const pnode_root_
) {
	using namespace Urho3D;
	m_ppoly = ppoly_;
	Urho3D::Context* pcontext = pnode_root_->GetContext();
	m_pmodel = SharedPtr<Model>(new Model(pcontext));
	m_pvb = SharedPtr<VertexBuffer>(new VertexBuffer(pcontext));
	m_pib = SharedPtr<IndexBuffer>(new IndexBuffer(pcontext));
	m_pgeom = SharedPtr<Geometry>(new Geometry(pcontext));
	m_pnode = pnode_root_->CreateChild("Poly");
	m_pstatic_model = m_pnode->CreateComponent<StaticModel>();

	const Felt::Vec3f& fsize = m_ppoly->size().template cast<float>();
	const Felt::Vec3f& foffset = m_ppoly->offset().template cast<float>();
	const Vector3& pos_offset = reinterpret_cast<const Vector3&>(foffset);
	const Vector3& pos_min = pos_offset;
	const Vector3& pos_max = (reinterpret_cast<const Vector3&>(fsize) + pos_offset);

	m_pmodel->SetBoundingBox(BoundingBox(pos_min, pos_max));
	m_pmodel->SetNumGeometries(1);

	m_pnode->SetEnabled(false);

	m_pvb->SetShadowed(false);
	m_pib->SetShadowed(false);
}


bool GPUPoly::flush()
{
	using namespace Urho3D;
	if (m_ppoly->spxs().size() == 0)
	{
		m_pvb->Release();
		m_pib->Release();
		m_pnode->SetEnabled(false);
		return false;
	}

	m_pvb->SetSize(m_ppoly->vtxs().size(), MASK_POSITION|MASK_NORMAL);
	m_pib->SetSize(m_ppoly->spxs().size() * 3, true);
	m_pib->SetData(&m_ppoly->spxs()[0]);
	m_pvb->SetData(&m_ppoly->vtxs()[0]);
	m_pgeom->SetVertexBuffer(0, m_pvb);
	m_pgeom->SetIndexBuffer(m_pib);
	m_pgeom->SetDrawRange(TRIANGLE_LIST, 0, m_ppoly->spxs().size() * 3, false);
	m_pmodel->SetGeometry(0, 0, m_pgeom);
	m_pstatic_model->SetModel(m_pmodel);
	ResourceCache* cache = m_pnode->GetContext()->GetSubsystem<ResourceCache>();
	m_pstatic_model->SetMaterial(
		cache->GetResource<Material>("Materials/Surface.xml")
	);
	m_pstatic_model->SetCastShadows(true);
	m_pnode->SetEnabled(true);

	return true;
}
}
