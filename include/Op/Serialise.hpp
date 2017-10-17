#ifndef INCLUDE_OP_SERIALISE_HPP_
#define INCLUDE_OP_SERIALISE_HPP_

#include "Common.hpp"
#include "Base.hpp"

#include <Urho3D/Scene/Node.h>
#include <future>


namespace UrFelt
{
namespace Op
{
namespace Serialise
{

class Load
{
public:
	static Load load(const std::string& file_path_, Urho3D::Node* pnode_);
	bool ready();
	std::unique_ptr<UrSurface> get();

private:
	Load(const std::string& file_path_, Urho3D::Node* pnode_);
	Urho3D::Node* m_pnode;

	using Future = std::future<UrFelt::Surface> ;
	Future m_future;
};

class Save: public Base
{
public:
	Save(const std::string& file_path_);
	Save(const std::string& file_path_, sol::function callback_);
	void execute(UrSurface& surface);
private:
	const std::string m_file_path;
};

} /* namespace Serialise */
} /* namespace Op */
} /* namespace UrFelt */

#endif /* INCLUDE_OP_SERIALISE_HPP_ */
