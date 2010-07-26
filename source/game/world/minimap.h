// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MINIMAP_H_
#define _GLEST_GAME_MINIMAP_H_

#include <cassert>

#include "widgets.h"

using namespace Shared::Math;
using namespace Shared::Graphics;

namespace Glest { namespace Sim {

class World;

using namespace Widgets;

enum ExplorationState{
	esNotExplored,
	esExplored,
	esVisible
};

// =====================================================
// 	class Minimap
//
/// State of the in-game minimap
// =====================================================

class Minimap : public Widget, public MouseWidget {
private:
	Pixmap2D *fowPixmap0;
	Pixmap2D *fowPixmap1;
	Texture2D *tex;			// base map texture
	Texture2D *fowTex;		// Fog Of War texture
	Texture2D *unitsTex;	// Units 'overlay'
	bool fogOfWar, shroudOfDarkness;
	bool m_draggingCamera, m_draggingWidget;
	bool m_leftClickOrder, m_rightClickOrder;
	Vec2i moveOffset;

private:
	static const float exploredAlpha;
	static const Vec2i textureSize;

public:
	void init(int x, int y, const World *world, bool resuming);
	Minimap(bool FoW, Container::Ptr parent, Vec2i pos, Vec2i size);
	~Minimap();

	const Texture2D *getFowTexture() const	{return fowTex;}
	const Texture2D *getTexture() const		{return tex;}

	void resetFowTex();
	void updateFowTex(float t);
	// heavy use function
	void incFowTextureAlphaSurface(const Vec2i &sPos, float alpha) {
		assert(sPos.x < fowPixmap1->getW() && sPos.y < fowPixmap1->getH());
	
		if(fowPixmap1->getPixelf(sPos.x, sPos.y) < alpha) {
			fowPixmap1->setPixel(sPos.x, sPos.y, alpha);
		}
	}

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);

	virtual void render();
	virtual string desc() { return string("[MiniMap: ") + descPosDim() + "]"; }

	sigslot::signal<Vec2i> LeftClickOrder;
	sigslot::signal<Vec2i> RightClickOrder;

	void setLeftClickOrder(bool enable) { m_leftClickOrder = enable; }
	void setRightClickOrder(bool enable) { m_rightClickOrder = enable; }

private:
	void computeTexture(const World *world);
	void setExploredState(const World *world);
};

}}//end namespace

#endif
