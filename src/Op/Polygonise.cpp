#include <Op/Polygonise.hpp>

namespace UrFelt
{
namespace Op
{
namespace Polygonise
{


Global::Global(sol::function callback_) :
	Base{callback_}
{}


void Global::execute(UrSurface& surface)
{
	if (this->m_cancelled)
		return;

	surface.polygonise();
}

} // Polygonise.
} // Op.
} // UrFelt.



