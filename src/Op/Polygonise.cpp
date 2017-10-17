#include "Op/Polygonise.hpp"

namespace UrFelt
{
namespace Op
{
namespace Polygonise
{


Global::Global() :
	Global{sol::function{}}
{}


Global::Global(sol::function callback_) :
	Base{callback_}
{}


void Global::execute(UrSurface& surface_)
{
	if (this->m_cancelled)
		return;

	surface_.polygonise();
}

} // Polygonise.
} // Op.
} // UrFelt.



