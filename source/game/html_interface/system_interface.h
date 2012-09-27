// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#ifndef _GLEST_GAME_SYSTEMINTERFACE_H_
#define _GLEST_GAME_SYSTEMINTERFACE_H_

#include "Core.h"

namespace Glest { namespace Gui {
// =====================================================
//	class MandateSystemInterface
// =====================================================

class MandateSystemInterface : public Rocket::Core::SystemInterface {
public:
    float GetElapsedTime();
};

}} //end namespace

#endif
