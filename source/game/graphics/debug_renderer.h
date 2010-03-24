// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2009	James McCulloch <silnarm at gmail>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#if ! _GAE_DEBUG_EDITION_
#	error debug_renderer.h included without _GAE_DEBUG_EDITION_
#endif

#ifndef _GLEST_GAME_DEBUG_RENDERER_
#define _GLEST_GAME_DEBUG_RENDERER_

#include "route_planner.h"   
#include "influence_map.h"
#include "cartographer.h"
#include "cluster_map.h"

#include "vec.h"
#include "math_util.h"
#include "pixmap.h"
#include "texture.h"
#include "graphics_factory_gl.h"
#include "scene_culler.h"
#include "game.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using Glest::Game::Search::InfluenceMap;
using Glest::Game::Search::ClusterMap;
using Glest::Game::Search::Cartographer;

namespace Glest { namespace Game {

class PathFinderTextureCallBack {
public:
	static set<Vec2i> pathSet, openSet, closedSet;
	static Vec2i pathStart, pathDest;
	static map<Vec2i,uint32> localAnnotations;
	
	static Field debugField;
	//static void loadPFDebugTextures ();
	static Texture2D *PFDebugTextures[26];

	Texture2DGl* operator()(const Vec2i &cell) {
		int ndx = -1;
		if (pathStart == cell) ndx = 9;
		else if (pathDest == cell) ndx = 10;
		else if (pathSet.find(cell) != pathSet.end()) ndx = 14; // on path
		else if (closedSet.find(cell) != closedSet.end()) ndx = 16; // closed nodes
		else if (openSet.find(cell) != openSet.end()) ndx = 15; // open nodes
		else if (localAnnotations.find(cell) != localAnnotations.end()) // local annotation
			ndx = 17 + localAnnotations.find(cell)->second;
		else ndx = theWorld.getCartographer()->getMasterMap()->metrics[cell].get(debugField); // else use cell metric for debug field
		return (Texture2DGl*)PFDebugTextures[ndx];
   }
};

class GridTextureCallback {
public:
	static Texture2D *tex;
	Texture2DGl* operator()(const Vec2i &cell) {
		return (Texture2DGl*)tex;
   }
};

class RegionHilightCallback {
public:
	static set<Vec2i> blueCells, greenCells;

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		if (blueCells.find(cell) != blueCells.end()) {
			colour = Vec4f(0.f, 0.f, 1.f, 0.6f);
		} else if (greenCells.find(cell) != greenCells.end()) {
			colour = Vec4f(0.f, 1.f, 0.f, 0.6f);
		} else {
			return false;
		}
		return true;
	}
};

class VisibleQuadColourCallback {
public:
	static set<Vec2i> quadSet;
	static Vec4f colour;

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		if (quadSet.find(cell) == quadSet.end()) {
			return false;
		}
		colour = this->colour;
		return true;
	}
};

class TeamSightColourCallback {
public:
	bool operator()(const Vec2i &cell, Vec4f &colour) {
		const Vec2i &tile = Map::toTileCoords(cell);
		int vis = theWorld.getCartographer()->getTeamVisibility(theWorld.getThisTeamIndex(), tile);
		if (!vis) {
			return false;
		}
		colour = Vec4f(0.f, 0.f, 1.f, 0.3f);
		switch (vis) {
			case 1:  colour.w = 0.05f;	break;
			case 2:  colour.w = 0.1f;	break;
			case 3:  colour.w = 0.15f;	break;
			case 4:  colour.w = 0.2f;	break;
			case 5:  colour.w = 0.25f;	break;
		}
		return true;
	}
};

class PathfinderClusterOverlay {
public:
	static set<Vec2i> entranceCells;
	static set<Vec2i> pathCells;

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		const int &clusterSize = Search::clusterSize;
		if ( cell.x % clusterSize == clusterSize - 1 
		|| cell.y % clusterSize == clusterSize - 1  ) {
			if ( entranceCells.find(cell) != entranceCells.end() ) {
				colour = Vec4f(0.f, 1.f, 0.f, 0.7f); // entrance
			} else {
				colour = Vec4f(1.f, 0.f, 0.f, 0.7f);  // border
			}
		} else if ( pathCells.find(cell) != pathCells.end() ) { // intra-cluster edge
			colour = Vec4f(0.f, 0.f, 1.f, 0.7f);
		} else {
			return false; // nothing interesting
		}
		return true;
	}
};

class ResourceMapOverlay {
public:
	static const ResourceType *rt;

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		PatchMap<1> *pMap = theWorld.getCartographer()->getResourceMap(rt);
		if (pMap && pMap->getInfluence(cell)) {
			colour = Vec4f(1.f, 1.f, 0.f, 0.7f);
			return true;
		}
		return false;
	}
};

class StoreMapOverlay {
public:
	typedef vector<const Unit *> UnitList;
	static UnitList stores;

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		for (UnitList::iterator it = stores.begin(); it != stores.end(); ++it) {
			PatchMap<1> *pMap = theWorld.getCartographer()->getStoreMap(*it);
			if (pMap && pMap->getInfluence(cell)) {
				colour = Vec4f(0.f, 1.f, 0.3f, 0.7f);
				return true;
			}
		}
		return false;
	}
};

// =====================================================
// 	class DebugRender
//
/// Helper class compiled with _GAE_DEBUG_EDITION_ only
// =====================================================
class DebugRenderer {
private:
	set<Vec2i> clusterEdgesWest;
	set<Vec2i> clusterEdgesNorth;
	Vec3f frstmPoints[8];

public:
	DebugRenderer();
	void init();
	void commandLine(string &line);

	bool gridTextures, AAStarTextures, HAAStarOverlay, showVisibleQuad, captureVisibleQuad,
		regionHilights, teamSight, resourceMapOverlay, storeMapOverlay;
	bool captureFrustum;
	bool showFrustum;

private:
	template<typename CellTextureCallback>
	void renderCellTextures(SceneCuller &culler) {
		const Rect2i mapBounds(0, 0, theMap.getTileW()-1, theMap.getTileH()-1);
		float coordStep= theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		assertGl();

		glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);
		glEnable(GL_BLEND);
		glEnable(GL_COLOR_MATERIAL); 
		glDisable(GL_ALPHA_TEST);
		glActiveTexture( GL_TEXTURE0 );

		CellTextureCallback callback;

		SceneCuller::iterator it = culler.tile_begin();
		for ( ; it != culler.tile_end(); ++it ) {
			const Vec2i &pos= *it;
			int cx, cy;
			cx = pos.x * 2;
			cy = pos.y * 2;
			if(mapBounds.isInside(pos)){
				Tile *tc00 = theMap.getTile(pos.x, pos.y), *tc10 = theMap.getTile(pos.x+1, pos.y),
					 *tc01 = theMap.getTile(pos.x, pos.y+1), *tc11 = theMap.getTile(pos.x+1, pos.y+1);
				Vec3f tl = tc00->getVertex (), tr = tc10->getVertex (),
					  bl = tc01->getVertex (), br = tc11->getVertex ();
				Vec3f tc = tl + (tr - tl) / 2,  ml = tl + (bl - tl) / 2,
					  mr = tr + (br - tr) / 2, mc = ml + (mr - ml) / 2, bc = bl + (br - bl) / 2;
				Vec2i cPos(cx, cy);
				const Texture2DGl *tex = callback(cPos);
				renderCellTextured(tex, tc00->getNormal(), tl, tc, mc, ml);
				cPos = Vec2i(cx + 1, cy);
				tex = callback(cPos);
				renderCellTextured(tex, tc00->getNormal(), tc, tr, mr, mc);
				cPos = Vec2i(cx, cy + 1 );
				tex = callback(cPos);
				renderCellTextured(tex, tc00->getNormal(), ml, mc, bc, bl);
				cPos = Vec2i(cx + 1, cy + 1);
				tex = callback(cPos);
				renderCellTextured(tex, tc00->getNormal(), mc, mr, br, bc);
			}
		}
		//Restore
		glPopAttrib();
		//assert
		glGetError();	//remove when first mtex problem solved
		assertGl();

	} // renderCellTextures ()

	template< typename CellOverlayColourCallback >
	void renderCellOverlay(SceneCuller &culler) {
		const Rect2i mapBounds( 0, 0, theMap.getTileW() - 1, theMap.getTileH() - 1 );
		float coordStep = theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		Vec4f colour;
		assertGl();
		glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT );
		glEnable( GL_BLEND );
		glEnable( GL_COLOR_MATERIAL ); 
		glDisable( GL_ALPHA_TEST );
		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );

		CellOverlayColourCallback callback;

		SceneCuller::iterator it = culler.tile_begin();
		for ( ; it != culler.tile_end(); ++it ) {
			const Vec2i &pos= *it;
			int cx, cy;
			cx = pos.x * 2;
			cy = pos.y * 2;
			if ( mapBounds.isInside( pos ) ) {
				Tile *tc00= theMap.getTile(pos.x, pos.y),	*tc10= theMap.getTile(pos.x+1, pos.y),
					 *tc01= theMap.getTile(pos.x, pos.y+1),	*tc11= theMap.getTile(pos.x+1, pos.y+1);
				Vec3f tl = tc00->getVertex(),	tr = tc10->getVertex(),
					  bl = tc01->getVertex(),	br = tc11->getVertex(); 
				tl.y += 0.1f; tr.y += 0.1f; bl.y += 0.1f; br.y += 0.1f;
				Vec3f tc = tl + (tr - tl) / 2,	ml = tl + (bl - tl) / 2,	mr = tr + (br - tr) / 2,
					  mc = ml + (mr - ml) / 2,	bc = bl + (br - bl) / 2;

				if (callback(Vec2i(cx,cy), colour)) {
					renderCellOverlay(colour, tc00->getNormal(), tl, tc, mc, ml);
				}
				if (callback(Vec2i(cx+1, cy), colour)) {
					renderCellOverlay(colour, tc00->getNormal(), tc, tr, mr, mc);
				}
				if (callback(Vec2i(cx, cy + 1), colour)) {
					renderCellOverlay(colour, tc00->getNormal(), ml, mc, bc, bl);
				}
				if (callback(Vec2i(cx + 1, cy + 1), colour)) {
					renderCellOverlay(colour, tc00->getNormal(), mc, mr, br, bc);
				}
			}
		}
		//Restore
		glPopAttrib();
		assertGl();
	}

	void renderClusterOverlay(SceneCuller &culler) {
		renderCellOverlay<PathfinderClusterOverlay>(culler);
	}
	void renderRegionHilight(SceneCuller &culler) {
		renderCellOverlay<RegionHilightCallback>(culler);
	}
	void renderCapturedQuad(SceneCuller &culler) {
		renderCellOverlay<VisibleQuadColourCallback>(culler);
	}
	void renderTeamSightOverlay(SceneCuller &culler) {
		renderCellOverlay<TeamSightColourCallback>(culler);
	}
	void renderResourceMapOverlay(SceneCuller &culler) {
		renderCellOverlay<ResourceMapOverlay>(culler);
	}
	void renderStoreMapOverlay(SceneCuller &culler) {
		if (!StoreMapOverlay::stores.empty()) {
			renderCellOverlay<StoreMapOverlay>(culler);
		}
	}
	void renderCellTextured(const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3);
	void renderCellOverlay(const Vec4f colour,  const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3);
	void renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width);

	static list<Vec3f> waypoints;
	void renderGrid(SceneCuller &culler) {
		renderCellTextures<GridTextureCallback>(culler);
	}

	void renderPathOverlay();
	void renderIntraClusterEdges(const Vec2i &cluster, CardinalDir dir = CardinalDir::COUNT);

	void renderFrustum() const;

public:
	static void clearWaypoints()		{ waypoints.clear();		}
	static void addWaypoint(Vec3f v)	{ waypoints.push_back(v);	}

	void sceneEstablished(SceneCuller &culler);
	bool willRenderSurface() const { return AAStarTextures || gridTextures; }
	void renderSurface(SceneCuller &culler) {
		if (AAStarTextures) {
			if (gridTextures) gridTextures = false;
			renderCellTextures<PathFinderTextureCallBack>(culler);
		} else if (gridTextures) {
			renderCellTextures<GridTextureCallback>(culler);
		}
	}
	void renderEffects(SceneCuller &culler);
};

}}

#endif
