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

#include <Common.hpp>
#include <Felt/Surface.hpp>
#include <Felt/Polys.hpp>


namespace UrFelt
{

class GPUPoly
{
public:
	void bind (
		const UrFelt::Polys::Child *const ppoly_,
		Urho3D::Context *const pcontext_, Urho3D::Node *const pnode_root_
	);

	bool flush ();

private:
	const UrFelt::Polys::Child*	m_ppoly;
	Urho3D::Context*	m_pcontext;
	Urho3D::SharedPtr<Urho3D::Model>	m_pmodel;
	Urho3D::SharedPtr<Urho3D::VertexBuffer>	m_pvb;
	Urho3D::SharedPtr<Urho3D::IndexBuffer>	m_pib;
	Urho3D::SharedPtr<Urho3D::Geometry>	m_pgeom;
	Urho3D::Node*			m_pnode;
	Urho3D::StaticModel*	m_pstatic_model;
};


}
