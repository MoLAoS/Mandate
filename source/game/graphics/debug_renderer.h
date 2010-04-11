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
//namespace Game { namespace Debug {

class PathFinderTextureCallback {
public:
	set<Vec2i> pathSet, openSet, closedSet;
	Vec2i pathStart, pathDest;
	map<Vec2i,uint32> localAnnotations;
	Field debugField;
	Texture2D *PFDebugTextures[26];

	PathFinderTextureCallback();

	void reset();
	void loadTextures();

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
	Texture2D *tex;

	void reset() { if (tex) tex->end(); tex = 0; }
	void loadTextures();

	GridTextureCallback() : tex(0) {}

	Texture2DGl* operator()(const Vec2i &cell) {
		return (Texture2DGl*)tex;
   }
};

WRAPPED_ENUM( HighlightColour,
	BLUE,
	GREEN
);

class CellHighlightOverlay {
public:
	typedef map<Vec2i, HighlightColour> CellColours;
	CellColours cells;

	Vec4f highlightColours[HighlightColour::COUNT];

	CellHighlightOverlay() {
		highlightColours[HighlightColour::BLUE] = Vec4f(0.f, 0.f, 1.f, 0.6f);
		highlightColours[HighlightColour::GREEN] = Vec4f(0.f, 1.f, 0.f, 0.6f);
	}

	void reset() {
		cells.clear();
	}

	bool empty() const { return cells.empty(); }

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		CellColours::iterator it = cells.find(cell);
		if (it != cells.end()) {
			colour = highlightColours[it->second];
			return true;
		}
		return false;
	}
};

class VisibleAreaOverlay {
public:
	set<Vec2i> quadSet;
	Vec4f colour;

	void reset() {
		colour = Vec4f(0.f, 1.f, 0.f, 0.5f);
		quadSet.clear();
	}

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		if (quadSet.find(cell) == quadSet.end()) {
			return false;
		}
		colour = this->colour;
		return true;
	}
};

class TeamSightOverlay {
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

class ClusterMapOverlay {
public:
	set<Vec2i> entranceCells;
	set<Vec2i> pathCells;

	void reset() {
		entranceCells.clear();
		pathCells.clear();
	}

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
	const ResourceType *rt;

	ResourceMapOverlay() : rt(0) {}
	void reset() { rt = 0; }

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
	UnitList stores;

	void reset() { stores.clear(); }

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

class BuildSiteMapOverlay {
public:
	set<Vec2i> cells;

	void reset() { cells.clear(); }

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		if (cells.find(cell) != cells.end()) {
			colour = Vec4f(0.f, 1.f, 0.3f, 0.7f);
			return true;
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

	PathFinderTextureCallback	pfCallback;
	GridTextureCallback			gtCallback;
	CellHighlightOverlay		rhCallback;
	VisibleAreaOverlay			vqCallback;
	TeamSightOverlay			tsCallback;
	ClusterMapOverlay			cmOverlay;
	ResourceMapOverlay			rmOverlay;
	StoreMapOverlay				smOverlay;
	BuildSiteMapOverlay			bsOverlay;

public:
	DebugRenderer();
	void init();
	void commandLine(string &line);

	bool	gridTextures,		// show cell grid
			AAStarTextures,		// AA* search space and results of last low-level search visualisation
			HAAStarOverlay,		// HAA* search space and results of last hierarchical search visualisation
			showVisibleQuad,	// set to show visualisation of last captured scene cull
			captureVisibleQuad, // set to trigger a capture of the next scene cull
			captureFrustum,		// set to trigger a capture of the view frustum
			showFrustum,		// set to show visualisation of captured view frustum
			regionHilights,		// show hilighted cells, are, and can further be, used for various things
			teamSight,			// currently useless ;)
			resourceMapOverlay,	// show resource goal map overlay
			storeMapOverlay,	// show store goal map overlay
			buildSiteMaps;		// show building site goal maps

	void addCellHighlight(const Vec2i &pos, HighlightColour c = HighlightColour::BLUE) {
		rhCallback.cells[pos] = c;
	}

	void clearCellHilights() {
		rhCallback.cells.clear();
	}

	void addBuildSiteCell(const Vec2i &pos) {
		bsOverlay.cells.insert(pos);
	}

	PathFinderTextureCallback& getPFCallback() { return pfCallback; }
	ClusterMapOverlay&	getCMOverlay() { return cmOverlay; }

private:
	/***/
	template<typename TextureCallback>
	void renderCellTextures(SceneCuller &culler, TextureCallback callback) {
		const Rect2i mapBounds(0, 0, theMap.getTileW()-1, theMap.getTileH()-1);
		float coordStep= theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		assertGl();

		glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);
		glEnable(GL_BLEND);
		glEnable(GL_COLOR_MATERIAL); 
		glDisable(GL_ALPHA_TEST);
		glActiveTexture( GL_TEXTURE0 );

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

	
	/***/
	template< typename ColourCallback >
	void renderCellOverlay(SceneCuller &culler, ColourCallback callback) {
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

	/***/
	void renderCellTextured(const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3);

	/***/
	void renderCellOverlay(const Vec4f colour,  const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3);
	
	/***/
	void renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width);

	/***/
	void renderPathOverlay();

	/***/
	void renderIntraClusterEdges(const Vec2i &cluster, CardinalDir dir = CardinalDir::COUNT);

	/***/
	void renderFrustum() const;

	list<Vec3f> waypoints;

public:
	void clearWaypoints()		{ waypoints.clear();		}
	void addWaypoint(Vec3f v)	{ waypoints.push_back(v);	}

	void sceneEstablished(SceneCuller &culler);
	bool willRenderSurface() const { return AAStarTextures || gridTextures; }
	void renderSurface(SceneCuller &culler) {
		if (AAStarTextures) {
			if (gridTextures) gridTextures = false;
			renderCellTextures(culler, pfCallback);
		} else if (gridTextures) {
			renderCellTextures(culler, gtCallback);
		}
	}
	void renderEffects(SceneCuller &culler);
};

}}

#endif
