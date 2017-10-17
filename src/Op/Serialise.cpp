#include "Op/Serialise.hpp"

#include <UrSurface.hpp>
#include <chrono>

namespace UrFelt
{
namespace Op
{
namespace Serialise
{

Load Load::load(const std::string& file_path_, Urho3D::Node* pnode_)
{
	return Load{file_path_, pnode_};
}


Load::Load(const std::string& file_path_, Urho3D::Node* pnode_) :
	m_pnode{pnode_}
{
	m_future = std::async(
		std::launch::async,
		[file_path_]() {
			return UrFelt::Surface::load(file_path_);
		}
	);
}


bool Load::ready()
{
	return m_future.wait_for(std::chrono::nanoseconds::zero()) == std::future_status::ready;
}


std::unique_ptr<UrSurface> Load::get()
{
	return std::make_unique<UrSurface>(std::move(m_future.get()), m_pnode);
}


Save::Save(const std::string& file_path_) :
	Save{file_path_, sol::function{}}
{}


Save::Save(const std::string& file_path_, sol::function callback_) :
	Base{callback_}, m_file_path{file_path_}
{}


void Save::execute(UrSurface& surface)
{
	surface.save(m_file_path);
}

} /* namespace Serialise */
} /* namespace Op */
} /* namespace UrFelt */
