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

#include "vec.h"
#include "math_util.h"
#include "pixmap.h"
#include "texture.h"
#include "graphics_factory_gl.h"

#include "route_planner.h"   
#include "influence_map.h"
#include "cartographer.h"
#include "cluster_map.h"

#include "scene_culler.h"
#include "game.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using namespace Glest::Search;
using Glest::Search::StoreMapKey;
using Glest::Search::StoreMapKey;

namespace Glest { namespace Debug {

extern bool reportRenderUnitsFlag;

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
	Texture2DGl* operator()(const Vec2i &cell);
};

class GridTextureCallback {
public:
	Texture2D *tex;

	void reset() { tex = 0; }
	void loadTextures();

	GridTextureCallback() : tex(0) {}

	Texture2DGl* operator()(const Vec2i &cell) {
		return (Texture2DGl*)tex;
   }
};

class CellHighlightOverlay {
public:
	typedef map<Vec2i, Vec4f> CellColours;
	CellColours cells;

	CellHighlightOverlay() {}

	void reset() {
		cells.clear();
	}

	bool empty() const { return cells.empty(); }

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		CellColours::iterator it = cells.find(cell);
		if (it != cells.end()) {
			colour = it->second;
			return true;
		}
		return false;
	}
};

class InfluenceMapOverlay {
public:
	TypeMap<float> *iMap;
	Vec3f baseColour;
	float max;

	bool operator()(const Vec2i &cell, Vec4f &colour) {
		const float influence = iMap->getInfluence(cell);
		if (influence != 0.f) {
			colour = Vec4f(baseColour, clamp(influence / max, 0.f, 1.f));
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
	bool operator()(const Vec2i &cell, Vec4f &colour);
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
		const int &clusterSize = GameConstants::clusterSize;
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

	bool operator()(const Vec2i &cell, Vec4f &colour);
};

class StoreMapOverlay {
public:
	typedef vector<StoreMapKey> KeyList;
	KeyList storeMaps;

	void reset() { storeMaps.clear(); }

	bool operator()(const Vec2i &cell, Vec4f &colour);
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
	InfluenceMapOverlay			imOverlay;

public:
	DebugRenderer();
	void reset();
	void init();
	void commandLine(string &line);

	bool	gridTextures,		// show cell grid
			AAStarTextures,		// AA* search space and results of last low-level search
			HAAStarOverlay,		// HAA* search space and results of last hierarchical search
			showVisibleQuad,	// set to show visualisation of last captured scene cull
			captureVisibleQuad, // set to trigger a capture of the next scene cull
			captureFrustum,		// set to trigger a capture of the view frustum
			showFrustum,		// set to show visualisation of captured view frustum
			regionHilights,		// show hilighted cells, are, and can further be, used for various things
			teamSight,			// currently useless ;)
			resourceMapOverlay,	// show resource goal map overlay
			storeMapOverlay,	// show store goal map overlay
			buildSiteMaps,		// show building site goal maps
			influenceMap;		// visualise an inluence map, [TypeMap<float> only]

	void addCellHighlight(const Vec2i &pos, Vec4f c = Vec4f(0.f, 0.f, 1.f, 0.3f)) {
		rhCallback.cells[pos] = c;
	}

	void clearCellHilights() {
		rhCallback.cells.clear();
	}

	void addBuildSiteCell(const Vec2i &pos) {
		bsOverlay.cells.insert(pos);
	}

	void setInfluenceMap(TypeMap<float> *iMap, Vec3f colour, float max);

	PathFinderTextureCallback& getPFCallback() { return pfCallback; }
	ClusterMapOverlay&	getCMOverlay() { return cmOverlay; }

private:
	/***/
	template<typename TextureCallback>
	void renderCellTextures(SceneCuller &culler, TextureCallback &callback) {
		const Rect2i mapBounds(0, 0, g_map.getTileW() - 1, g_map.getTileH() - 1);
		MapVertexData &mapData = *g_map.getVertexData();
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
			if (mapBounds.isInside(pos)) {
				Vec3f	tl = mapData.get(pos.x + 0, pos.y + 0).vert(),
						tr = mapData.get(pos.x + 1, pos.y + 0).vert(),
						bl = mapData.get(pos.x + 0, pos.y + 1).vert(),
						br = mapData.get(pos.x + 1, pos.y + 1).vert();
				Vec3f	tc = tl + (tr - tl) / 2,
						ml = tl + (bl - tl) / 2,
						mr = tr + (br - tr) / 2,
						mc = ml + (mr - ml) / 2,
						bc = bl + (br - bl) / 2;
				Vec3f	norm = mapData.get(pos).norm();
				Vec2i cPos(cx, cy);
				const Texture2DGl *tex = callback(cPos);
				renderCellTextured(tex, norm, tl, tc, mc, ml);
				cPos = Vec2i(cx + 1, cy);
				tex = callback(cPos);
				renderCellTextured(tex, norm, tc, tr, mr, mc);
				cPos = Vec2i(cx, cy + 1 );
				tex = callback(cPos);
				renderCellTextured(tex, norm, ml, mc, bc, bl);
				cPos = Vec2i(cx + 1, cy + 1);
				tex = callback(cPos);
				renderCellTextured(tex, norm, mc, mr, br, bc);
			}
		}
		//Restore
		glPopAttrib();
		//assert
		assertGl();

	} // renderCellTextures ()

	
	/***/
	template< typename ColourCallback >
	void renderCellOverlay(SceneCuller &culler, ColourCallback &callback) {
		const Rect2i mapBounds( 0, 0, g_map.getTileW() - 1, g_map.getTileH() - 1 );
		MapVertexData &mapData = *g_map.getVertexData();
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
				Vec3f	tl = mapData.get(pos.x + 0, pos.y + 0).vert(),
						tr = mapData.get(pos.x + 1, pos.y + 0).vert(),
						bl = mapData.get(pos.x + 0, pos.y + 1).vert(),
						br = mapData.get(pos.x + 1, pos.y + 1).vert();
				
				tl.y += 0.1f; tr.y += 0.1f; bl.y += 0.1f; br.y += 0.1f;

				Vec3f	tc = tl + (tr - tl) / 2,
						ml = tl + (bl - tl) / 2,
						mr = tr + (br - tr) / 2,
						mc = ml + (mr - ml) / 2,
						bc = bl + (br - bl) / 2;
				Vec3f	norm = mapData.get(pos).norm();

				if (callback(Vec2i(cx,cy), colour)) {
					renderCellOverlay(colour, norm, tl, tc, mc, ml);
				}
				if (callback(Vec2i(cx+1, cy), colour)) {
					renderCellOverlay(colour, norm, tc, tr, mr, mc);
				}
				if (callback(Vec2i(cx, cy + 1), colour)) {
					renderCellOverlay(colour, norm, ml, mc, bc, bl);
				}
				if (callback(Vec2i(cx + 1, cy + 1), colour)) {
					renderCellOverlay(colour, norm, mc, mr, br, bc);
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

DebugRenderer& getDebugRenderer();

}}

#endif
