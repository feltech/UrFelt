#include "UrPoly3D.hpp"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/Log.h>

namespace felt
{


UrPoly3D::UrPoly3D ()
: Base(), m_pmodel(), m_pvb(), m_pib(), m_pgeom(), m_pnode(NULL),
  m_pstatic_model(NULL)
{}


UrPoly3D::UrPoly3D (
	const Vec3u& dims_, const Vec3i& offset_
) : Base(dims_, offset_), m_pmodel(), m_pvb(), m_pib(), m_pgeom(),
	m_pnode(NULL), m_pstatic_model(NULL), m_pcontext(NULL)
{}


UrPoly3D::~UrPoly3D ()
{}


void UrPoly3D::init_gpu (
	const Vec3u& dims_, const Vec3i& offset_, Urho3D::Context* pcontext_,
	Urho3D::Node* pnode_root_
) {
	using namespace Urho3D;
	m_pcontext = pcontext_;
	m_pmodel = SharedPtr<Model>(new Model(pcontext_));
	m_pvb = SharedPtr<VertexBuffer>(new VertexBuffer(pcontext_));
	m_pib = SharedPtr<IndexBuffer>(new IndexBuffer(pcontext_));
	m_pgeom = SharedPtr<Geometry>(new Geometry(pcontext_));
	m_pnode = pnode_root_->CreateChild("Poly");
	m_pstatic_model = m_pnode->CreateComponent<StaticModel>();

	felt::Vec3f fdims = dims_.template cast<float>();
	felt::Vec3f foffset = offset_.template cast<float>();
	Vector3 pos_offset = reinterpret_cast<Vector3&>(foffset);
	Vector3 pos_min = pos_offset;
	Vector3 pos_max = (reinterpret_cast<Vector3&>(fdims) + pos_offset);

	m_pmodel->SetBoundingBox(BoundingBox(pos_min, pos_max));
	m_pmodel->SetNumGeometries(1);

	m_pnode->SetEnabled(false);

	m_pvb->SetShadowed(false);
	m_pib->SetShadowed(false);
}


bool UrPoly3D::update_gpu()
{
	using namespace Urho3D;
	if (this->spx().size() == 0)
	{
		m_pvb->Release();
		m_pib->Release();
		m_pnode->SetEnabled(false);
		return false;
	}

	m_pvb->SetSize(this->vtx().size(), MASK_POSITION|MASK_NORMAL);
	m_pib->SetSize(this->spx().size() * 3, true);
	m_pib->SetData(&this->spx()[0]);
	m_pvb->SetData(&this->vtx()[0]);
	m_pgeom->SetVertexBuffer(0, m_pvb);
	m_pgeom->SetIndexBuffer(m_pib);
	m_pgeom->SetDrawRange(TRIANGLE_LIST, 0, this->spx().size() * 3, false);
	m_pmodel->SetGeometry(0, 0, m_pgeom);
	m_pstatic_model->SetModel(m_pmodel);
	ResourceCache* cache = m_pcontext->GetSubsystem<ResourceCache>();
	m_pstatic_model->SetMaterial(
		cache->GetResource<Material>("Materials/Surface.xml")
	);
	m_pstatic_model->SetCastShadows(true);
	m_pnode->SetEnabled(true);

	return true;
}
}
