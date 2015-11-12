#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Scene/Scene.h>

#include <Felt/Surface.hpp>
#include <Felt/Poly.hpp>

#include <GL/gl.h>

namespace felt
{

class UrPoly3D : public Poly<3>
{
protected:
	typedef Poly<3>	Base;

	Urho3D::SharedPtr<Urho3D::Model> m_pmodel;
	Urho3D::SharedPtr<Urho3D::VertexBuffer> m_pvb;
	Urho3D::SharedPtr<Urho3D::IndexBuffer> m_pib;
	Urho3D::SharedPtr<Urho3D::Geometry> m_pgeom;
	Urho3D::Node*			m_pnode;
	Urho3D::StaticModel*	m_pstatic_model;
	Urho3D::Context* 		m_pcontext;
    
public:
	~UrPoly3D();
	UrPoly3D ();

	UrPoly3D (const Vec3u& dims_, const Vec3i& offset_);

	void init_gpu (
		const Vec3u& dims_, const Vec3i& offset_, Urho3D::Context* pcontext_,
		Urho3D::Node* pnode_root_
	);

	bool update_gpu ();
};
  
  
}
