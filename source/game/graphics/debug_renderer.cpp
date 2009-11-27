#include "pch.h"
#include "renderer.h"

#if DEBUG_RENDERING_ENABLED

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Glest { namespace Game{

set<Vec2i>	RegionHilightCallback::cells; 

}} // end namespace Glest::Game

#endif // DEBUG_RENDERING_ENABLED
