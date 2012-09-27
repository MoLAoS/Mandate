// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#include "pch.h"
#include "system_interface.h"
#include "world.h"

namespace Glest { namespace Gui {
// =====================================================
//	class MandateSystemInterface
// =====================================================

float MandateSystemInterface::GetElapsedTime() {
    float timeElapsed = g_world.getFrameCount() / 40;
    return timeElapsed;
}

}} //end namespace
