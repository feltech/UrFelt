#ifndef INCLUDE_OP_Polygonise_HPP_
#define INCLUDE_OP_Polygonise_HPP_

#include "Op/Base.hpp"
#include "UrSurface.hpp"

namespace UrFelt
{
namespace Op
{
namespace Polygonise
{

struct Global : Base
{
	Global();
	Global(sol::function callback_);
	void execute(UrSurface& surface);
};

} // Polygonise
} // Op.
} // UrFelt.

#endif /* INCLUDE_OP_Polygonise_HPP_ */
