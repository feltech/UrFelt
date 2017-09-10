#ifndef INCLUDE_COMMON_HPP_
#define INCLUDE_COMMON_HPP_

#include <Felt/Surface.hpp>
#include <Felt/Polys.hpp>

namespace UrFelt
{
using Surface = Felt::Surface<3, 3>;
using IsoGrid = Surface::IsoGrid;
using Polys = Felt::Polys<Surface>;
}


#endif /* INCLUDE_COMMON_HPP_ */
