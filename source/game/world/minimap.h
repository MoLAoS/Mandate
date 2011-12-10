// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010      James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MINIMAP_H_
#define _GLEST_GAME_MINIMAP_H_

#include <cassert>

#include "framed_widgets.h"
#include "influence_map.h"

using namespace Shared::Math;
using namespace Shared::Graphics;

namespace Glest { namespace Sim {

class World;
using namespace Search;
using namespace Widgets;

enum ExplorationState{
	esNotExplored,
	esExplored,
	esVisible
};

struct AttackNoticeCircle {
	Vec2i	m_pos;
	float	m_alpha;
	int		m_ttl;
	bool	m_up;

	AttackNoticeCircle() {}
	void init(Vec2i pos);

	bool update();
	void render(Vec2i mmPos, int mmHeight, fixed ratio);
};

// =====================================================
// 	class Minimap
//
/// State of the in-game minimap
// =====================================================

class Minimap : public Widget, public MouseWidget {
private:
	typedef vector<AttackNoticeCircle> AttackNotices;

private:
	Pixmap2D*		m_fowPixmap0;
	Pixmap2D*		m_fowPixmap1;
	Texture2D*		m_terrainTex;	// base map texture
	Texture2D*		m_fowTex;		// Fog Of War texture
	Texture2D*		m_unitsTex;		// Units 'overlay' texture
	TypeMap<int8>*	m_unitsPMap;	// overlay construction helper (at cell resolution)

	Texture2D*		m_attackNoticeTex;
	AttackNotices	m_attackNotices;

	int		m_w, m_h;
	fixed	m_ratio,	/**< aspect ratio of map */
			m_currZoom,	/**< current zoom level at cell scale */
			m_maxZoom,	/**< maximum zoom level */
			m_minZoom;	/**< minimum zoom level */

	bool	m_fogOfWar, 
			m_shroudOfDarkness,

			m_draggingCamera,
			//m_draggingWidget,
			m_leftClickOrder,
			m_rightClickOrder;

	//Vec2i	m_moveOffset;

private:
	static const float exploredAlpha;
	static const Vec2i textureSize;

private:
	Vec2i toCellPos(Vec2i mmPos) const;/* { 
		return Vec2i((mmPos.x / m_currZoom).intp(), (mmPos.y / m_currZoom).intp());
	}*/
	Vec2i toMiniMapPos(Vec2i cellPos) const;/* {
		return Vec2i((cellPos.x * m_currZoom).intp(), (cellPos.y * m_currZoom).intp());
	}*/

public:
	void init(int x, int y, const World *world, bool resuming);
	Minimap(bool FoW, bool SoD, Container* parent, Vec2i pos);
	~Minimap();

	const Texture2D *getFowTexture() const	{return m_fowTex;}
	const Texture2D *getTexture() const		{return m_terrainTex;}

	void addAttackNotice(Vec2i pos);

	void update(int frameCount);

	void setMinimapSize(FuzzySize size);
	FuzzySize getMinimapSize() const;

	void resetFowTex();          // swap FoW pixmaps, resetting values on the now 'old' one
	void updateFowTex(float t);  // update FoW tex, interpolating between pixmaps (according to 't')
	void updateUnitTex();

	// heavy use function
	void incFowTextureAlphaSurface(const Vec2i &sPos, float alpha) {
		assert(sPos.x < m_fowPixmap1->getW() && sPos.y < m_fowPixmap1->getH());
		if (m_fowPixmap1->getPixelf(sPos.x, sPos.y) < alpha) {
			m_fowPixmap1->setPixel(sPos.x, sPos.y, alpha);
		}
	}

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);

	virtual void render();
	virtual string descType() const override { return "MiniMap"; }

	sigslot::signal<Vec2i> LeftClickOrder;
	sigslot::signal<Vec2i> RightClickOrder;

	void setLeftClickOrder(bool enable) { m_leftClickOrder = enable; }
	void setRightClickOrder(bool enable) { m_rightClickOrder = enable; }

	void persist();
	void reset();

private:
	void computeTerrainTexture(const World *world);  // init terrain tex
	void setExploredState(const World *world);       // init FoW pixmaps
};

// =====================================================
//  class MinimapFrame
// =====================================================

class MinimapFrame : public Frame {
private:
	Minimap  *m_minimap;

	void doEnableShrinkExpand(FuzzySize sz);

public:
	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	MinimapFrame(Container *parent, Vec2i pos, bool FoW, bool SoD);
	void initMinimp(FuzzySize sz, int w, int h, const World *world, bool resumingGame);

	Minimap* getMinimap() {return m_minimap;}

	void setPinned(bool v);

	virtual void render() override;
};

}}//end namespace

#endif
