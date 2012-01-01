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

#include "pch.h"

#if _GAE_DEBUG_EDITION_

#include "debug_renderer.h"
#include "route_planner.h"   
#include "influence_map.h"
#include "cartographer.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;
using Glest::Search::InfluenceMap;
using Glest::Search::Cartographer;

namespace Glest { namespace Debug {

bool reportRenderUnitsFlag = false;

// texture loading helper
void _load_debug_tex(Texture2D* &texPtr, const char *fileName) {
	texPtr = g_renderer.newTexture2D(ResourceScope::GAME);
	texPtr->setMipmap(false);
	texPtr->getPixmap()->load(fileName);
}

// =====================================================
//  class PathFinderTextureCallback
// =====================================================

PathFinderTextureCallback::PathFinderTextureCallback() 
		: debugField(Field::LAND) {
	reset(); 
}

void PathFinderTextureCallback::reset() {
	pathSet.clear();
	openSet.clear();
	closedSet.clear();
	pathStart = Vec2i(-1);
	pathDest = Vec2i(-1);
	localAnnotations.clear();
	debugField = Field::LAND;
	memset(PFDebugTextures, 0, sizeof(PFDebugTextures));
}

void PathFinderTextureCallback::loadTextures() {
#	define _load_tex(i,f) _load_debug_tex(PFDebugTextures[i],f)
	char buff[128];
	for (int i=0; i < 8; ++i) {
		sprintf(buff, "data/core/misc_textures/g%02d.bmp", i);
		_load_tex(i, buff);
	}
	_load_tex(9, "data/core/misc_textures/path_start.bmp");
	_load_tex(10, "data/core/misc_textures/path_dest.bmp");
	_load_tex(11, "data/core/misc_textures/path_both.bmp");
	_load_tex(12, "data/core/misc_textures/path_return.bmp");
	_load_tex(13, "data/core/misc_textures/path.bmp");

	_load_tex(14, "data/core/misc_textures/path_node.bmp");
	_load_tex(15, "data/core/misc_textures/open_node.bmp");
	_load_tex(16, "data/core/misc_textures/closed_node.bmp");

	for (int i=17; i < 17+8; ++i) {
		sprintf(buff, "data/core/misc_textures/l%02d.bmp", i-17);
		_load_tex(i, buff);
	}
#	undef _load_tex
}

Texture2DGl* PathFinderTextureCallback::operator()(const Vec2i &cell) {
	int ndx = -1;
	if (pathStart == cell) {
		ndx = 9;
	} else if (pathDest == cell) {
		ndx = 10;
	} else if (pathSet.find(cell) != pathSet.end()) {
		ndx = 14; // on path
	} else if (closedSet.find(cell) != closedSet.end()) {
		ndx = 16; // closed nodes
	} else if (openSet.find(cell) != openSet.end()) {
		ndx = 15; // open nodes
	} else if (localAnnotations.find(cell) != localAnnotations.end()) {
		ndx = 17 + localAnnotations.find(cell)->second; // local annotation
	} else { // else use cell metric for debug field
		const CellMetrics &metrics = g_cartographer.getMasterMap()->getMetrics()[cell];
		ndx = metrics.get(debugField);
	}
	return (Texture2DGl*)PFDebugTextures[ndx];
}

// =====================================================
//  class GridTextureCallback
// =====================================================

void GridTextureCallback::loadTextures() {
	_load_debug_tex(tex, "data/core/misc_textures/grid.bmp");
}

bool ResourceMapOverlay::operator()(const Vec2i &cell, Vec4f &colour) {
	ResourceMapKey mapKey(rt, Field::LAND, 1);
	PatchMap<1> *pMap = g_world.getCartographer()->getResourceMap(mapKey);
	if (pMap && pMap->getInfluence(cell)) {
		colour = Vec4f(1.f, 1.f, 0.f, 0.7f);
		return true;
	}
	return false;
}

bool StoreMapOverlay::operator()(const Vec2i &cell, Vec4f &colour) {
	foreach (KeyList, it, storeMaps) {
		PatchMap<1> *pMap = g_world.getCartographer()->getStoreMap(*it, false);
		if (pMap && pMap->getInfluence(cell)) {
			colour = Vec4f(0.f, 1.f, 0.3f, 0.7f);
			return true;
		}
	}
	return false;
}

bool TeamSightOverlay::operator()(const Vec2i &cell, Vec4f &colour) {
	const Vec2i &tile = Map::toTileCoords(cell);
	int vis = g_world.getCartographer()->getTeamVisibility(g_world.getThisTeamIndex(), tile);
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

// =====================================================
// 	class DebugRender
// =====================================================

DebugRenderer::DebugRenderer() {
	// defaults, listed for ease of maintenance. [Note: these can be set from Lua now, use debugSet()]
	showVisibleQuad = 
	captureVisibleQuad = 
	teamSight = 
	resourceMapOverlay = 
	storeMapOverlay =
	showFrustum = 
	captureFrustum = 
	gridTextures = 
	buildSiteMaps =
	influenceMap =
	AAStarTextures =
	HAAStarOverlay =

					false;

	regionHilights = 

					true;
}

const ResourceType* findResourceMapRes(const string &res) {
	const int &n = g_world.getTechTree()->getResourceTypeCount();
	for (int i=0; i < n; ++i) {
		const ResourceType *rt = g_world.getTechTree()->getResourceType(i);
		if (rt->getName() == res) {
			return rt;
		}
	}
	return 0;
}

void DebugRenderer::reset() {
	pfCallback.reset();
	gtCallback.reset();
	rhCallback.reset();
	vqCallback.reset();
	cmOverlay.reset();
	rmOverlay.reset();
	smOverlay.reset();
	bsOverlay.reset();
	rmOverlay.reset();
}

void DebugRenderer::init() {
	pfCallback.loadTextures();
	gtCallback.loadTextures();

	if (resourceMapOverlay) {
		string str("gold");
		rmOverlay.rt = findResourceMapRes(str);
	}
}

void DebugRenderer::sceneEstablished(SceneCuller &culler) {
	Vec4f blue(0.f, 0.f, 1.f, 0.5f);
	Vec4f green(0.f, 1.f, 0.f, 0.5f);
	if (captureFrustum) {
		captureFrustum = false;
		for (int i=0; i  < 8; ++i) {
			frstmPoints[i] = culler.frstmPoints[i];
		}

		for (int i=0; i < culler.boundingPoints.size(); ++i) {
			Vec2i pos(int(culler.boundingPoints[i].x), int(culler.boundingPoints[i].y));
			addCellHighlight(pos, blue);
		}

		vector<Vec2f>::iterator it = culler.visiblePoly.begin();
		for ( ; it != culler.visiblePoly.end(); ++it) {
			Vec2i pos(int(it->x), int(it->y));
			addCellHighlight(pos, green);
		}
		for ( int i=0; i < culler.cellExtrema.spans.size(); ++i) {
			int y = culler.cellExtrema.min_y + i;
			int x1 = culler.cellExtrema.spans[i].first;
			int x2 = culler.cellExtrema.spans[i].second;
			addCellHighlight(Vec2i(x1,y), green);
			addCellHighlight(Vec2i(x2,y), green);
		}
		showFrustum = true;
	}
}

void DebugRenderer::commandLine(string &line) {
	string key, val;
	size_t n = line.find('=');
	if ( n != string::npos ) {
		key = line.substr(0, n);
		val = line.substr(n+1);
	} else {
		key = line;
	}
	if ( key == "AStarTextures" ) {
		if ( val == "" ) { // no val supplied, toggle
			AAStarTextures = !AAStarTextures;
		} else {
			if ( val == "on" || val == "On" ) {
				AAStarTextures = true;
			} else {
				AAStarTextures = false;
			}
		}
	} else if ( key == "GridTextures" ) {
		if ( val == "" ) { // no val supplied, toggle
			gridTextures = !gridTextures;
		} else {
			if ( val == "on" || val == "On" ) {
				gridTextures = true;
			} else {
				gridTextures = false;
			}
		}
	} else if ( key == "ClusterOverlay" ) {
		if ( val == "" ) { // no val supplied, toggle
			HAAStarOverlay = !HAAStarOverlay;
		} else {
			if ( val == "on" || val == "On" ) {
				HAAStarOverlay = true;
			} else {
				HAAStarOverlay = false;
			}
		}
	} else if ( key == "CaptuereQuad" ) {
		captureVisibleQuad = true;
	} else if ( key == "RegionColouring" ) {
		if ( val == "" ) { // no val supplied, toggle
			regionHilights = !regionHilights;
		} else {
			if ( val == "on" || val == "On" ) {
				regionHilights = true;
			} else {
				regionHilights = false;
			}
		}
	} else if ( key == "DebugField" ) {
		Field f = FieldNames.match(val.c_str());
		if ( f != Field::INVALID ) {
			pfCallback.debugField = f;
		} else {
			g_console.addLine("Bad field: " + val);
		}
	} else if (key == "Frustum") {
		if (val == "capture" || val == "Capture") {
			captureFrustum = true;
		} else if (val == "off" || val == "Off") {
			showFrustum = false;
		}
	} else if (key == "RenderUnits") {
		reportRenderUnitsFlag = true;
	} else if (key == "ResourceMap") {
		if ( val == "" ) { // no val supplied, toggle
			resourceMapOverlay = !resourceMapOverlay;
		} else {
			const ResourceType *rt = 0;
			if ( val == "on" || val == "On" ) {
				resourceMapOverlay = true;
			} else if (val == "off" || val == "Off") {
				resourceMapOverlay = false;
			} else {
				// else find resource
				if (!( rt = findResourceMapRes(val))) {
					g_console.addLine("Error: value='" + val + "' not valid.");
					resourceMapOverlay = false;
				}
				resourceMapOverlay = true;
				rmOverlay.rt = rt;
			}
		}
	} else if (key == "StoreMap") {
		n = val.find(',');
		if (n == string::npos) {
			g_console.addLine("Error: value='" + val + "' not valid");
			return;
		}
		storeMapOverlay = false;
		string idString = val.substr(0, n);
		++n;
		while (val[n] == ' ') ++n;
		string szString = val.substr(n);
		int id, sz;
		try {
			id = Conversion::strToInt(idString);
			sz = Conversion::strToInt(szString);
		} catch (runtime_error &e) {
			g_console.addLine("Error: value='" + val + "' not valid: expected id, size (two integers)");
			return;
		}
		Unit *store = g_world.findUnitById(id);
		if (!store) {
			g_console.addLine("Error: unit id " + idString + " not found");
			return;
		}
		StoreMapKey smkey(store, Field::LAND, sz);
		PatchMap<1> *pMap = g_world.getCartographer()->getStoreMap(smkey, false);
		if (pMap) {
			smOverlay.storeMaps.push_back(smkey);
			storeMapOverlay = true;
		} else {
			g_console.addLine("Error: no StoreMap found for unit " + idString 
				+ " in Field::LAND with size " + szString);
		}
	} else if (key == "AssertClusterMap") {
		g_world.getCartographer()->getClusterMap()->assertValid();
	} else if (key == "TransitionEdges") {
		if (val == "clear") {
			clusterEdgesNorth.clear();
			clusterEdgesWest.clear();
		} else {
			n = val.find(',');
			if (n == string::npos) {
				g_console.addLine("Error: value=" + val + "not valid");
				return;
			}
			string xs = val.substr(0, n);
			val = val.substr(n + 1);
			int x = atoi(xs.c_str());
			n = val.find(':');
			if (n == string::npos) {
				g_console.addLine("Error: value=" + val + "not valid");
				return;
			}
			string ys = val.substr(0, n);
			val = val.substr(n + 1);
			int y = atoi(ys.c_str());
			if (val == "north") {
				clusterEdgesNorth.insert(Vec2i(x, y));
			} else if ( val == "west") {
				clusterEdgesWest.insert(Vec2i(x, y));
			} else if ( val == "south") {
				clusterEdgesNorth.insert(Vec2i(x, y + 1));
			} else if ( val == "east") {
				clusterEdgesWest.insert(Vec2i(x + 1, y));
			} else if ( val == "all") {
				clusterEdgesNorth.insert(Vec2i(x, y));
				clusterEdgesNorth.insert(Vec2i(x, y + 1));
				clusterEdgesWest.insert(Vec2i(x, y));
				clusterEdgesWest.insert(Vec2i(x + 1, y));
			} else {
				g_console.addLine("Error: value=" + val + "not valid");
			}
		}
	}
}

void DebugRenderer::renderCellTextured(const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
			const Vec3f &v1, const Vec3f &v2, const Vec3f &v3) {
	glBindTexture( GL_TEXTURE_2D, tex->getHandle() );
	glBegin( GL_TRIANGLE_FAN );
		glTexCoord2f( 0.f, 1.f );
		glNormal3fv( norm.ptr() );
		glVertex3fv( v0.ptr() );

		glTexCoord2f( 1.f, 1.f );
		glNormal3fv( norm.ptr() );
		glVertex3fv( v1.ptr() );

		glTexCoord2f( 1.f, 0.f );
		glNormal3fv( norm.ptr() );
		glVertex3fv( v2.ptr() );

		glTexCoord2f( 0.f, 0.f );
		glNormal3fv( norm.ptr() );
		glVertex3fv( v3.ptr() );                        
	glEnd ();
}

void DebugRenderer::renderCellOverlay(const Vec4f colour,  const Vec3f &norm, 
		const Vec3f &v0, const Vec3f &v1, const Vec3f &v2, const Vec3f &v3) {
	glBegin ( GL_TRIANGLE_FAN );
		glNormal3fv(norm.ptr());
		glColor4fv( colour.ptr() );
		glVertex3fv(v0.ptr());
		glNormal3fv(norm.ptr());
		glColor4fv( colour.ptr() );
		glVertex3fv(v1.ptr());
		glNormal3fv(norm.ptr());
		glColor4fv( colour.ptr() );
		glVertex3fv(v2.ptr());
		glNormal3fv(norm.ptr());
		glColor4fv( colour.ptr() );
		glVertex3fv(v3.ptr());                        
	glEnd ();
}

void DebugRenderer::renderArrow(
		const Vec3f &pos1, const Vec3f &_pos2, const Vec3f &color, float width) {
	const int tesselation = 3;
	const float arrowEndSize = 0.5f;
	float alphaFactor = 0.3f;

	Vec3f dir = Vec3f(_pos2 - pos1);
	dir.normalize();
	Vec3f pos2 = _pos2 - dir;
	Vec3f normal = dir.cross(Vec3f(0, 1, 0));

	Vec3f pos2Left  = pos2 + normal * (width - 0.05f) - dir * arrowEndSize * width;
	Vec3f pos2Right = pos2 - normal * (width - 0.05f) - dir * arrowEndSize * width;
	Vec3f pos1Left  = pos1 + normal * (width + 0.02f);
	Vec3f pos1Right = pos1 - normal * (width + 0.02f);

	//arrow body
	glBegin(GL_TRIANGLE_STRIP);
	for(int i=0; i<=tesselation; ++i){
		float t= static_cast<float>(i)/tesselation;
		Vec3f a= pos1Left.lerp(t, pos2Left);
		Vec3f b= pos1Right.lerp(t, pos2Right);
		Vec4f c= Vec4f(color, t*0.25f*alphaFactor);

		glColor4fv(c.ptr());
		glVertex3fv(a.ptr());
		glVertex3fv(b.ptr());
	}
	glEnd();

	//arrow end
	glBegin(GL_TRIANGLES);
		glVertex3fv((pos2Left + normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2Right - normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2 + dir*(arrowEndSize-0.1f)).ptr());
	glEnd();
}

void DebugRenderer::renderPathOverlay() {
	//return;
	Vec3f one, two;
	if ( waypoints.size() < 2 ) return;

	assertGl();
	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_COLOR_MATERIAL ); 
	glDisable( GL_ALPHA_TEST );
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glLineWidth(2.f);
	glActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );

	list<Vec3f>::iterator it = waypoints.begin(); 
	one = *it;
	++it;
	two = *it;
	while ( true ) {
		renderArrow(one,two,Vec3f(1.0f, 1.0f, 0.f), 0.15f);
		one = two;
		++it;
		if ( it == waypoints.end() ) break;
		two = *it;
	}
	//Restore
	glPopAttrib();
}

void DebugRenderer::renderIntraClusterEdges(const Vec2i &cluster, CardinalDir dir) {
	ClusterMap *cm = g_world.getCartographer()->getClusterMap();
	const Map *map = g_world.getMap();
	
	if (cluster.x < 0 || cluster.x >= cm->getWidth()
	|| cluster.y < 0 || cluster.y >= cm->getHeight()) {
		return;
	}

	Transitions transitions;
	if (dir != CardinalDir::COUNT) {
		TransitionCollection &tc = cm->getBorder(cluster, dir)->transitions[Field::LAND];
		for (int i=0; i < tc.n; ++i) {
			transitions.push_back(tc.transitions[i]);
		}
	} else {
		cm->getTransitions(cluster, Field::LAND, transitions);
	}		
	assertGl();
	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable( GL_COLOR_MATERIAL ); 
	glDisable( GL_ALPHA_TEST );
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glLineWidth(2.f);
	glActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );

	for (Transitions::iterator ti = transitions.begin(); ti != transitions.end(); ++ti) {
		const Transition* &t = *ti;
		float h = map->getCell(t->nwPos)->getHeight();
		Vec3f t1Pos(t->nwPos.x + 0.5f, h + 0.1f, t->nwPos.y + 0.5f);
		for (Edges::const_iterator ei = t->edges.begin(); ei != t->edges.end(); ++ei) {
			Search::Edge * const &e = *ei;
			//if (e->cost(1) != numeric_limits<float>::infinity()) {
				const Transition* t2 = e->transition();
				h = map->getCell(t2->nwPos)->getHeight();
				Vec3f t2Pos(t2->nwPos.x + 0.5f, h + 0.1f, t2->nwPos.y + 0.5f);
				renderArrow(t1Pos, t2Pos, Vec3f(1.f, 0.f, 1.f), 0.2f);
			//}
		}
	}
	//Restore
	glPopAttrib();
}

void DebugRenderer::renderFrustum() const {
	glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT );
	glEnable( GL_BLEND );
	glEnable( GL_COLOR_MATERIAL ); 
	glDisable( GL_ALPHA_TEST );
	glActiveTexture( GL_TEXTURE0 );
	glDisable( GL_TEXTURE_2D );
	
	glPointSize(5);
	glColor3f(1.f, 0.2f, 0.2f);
	glBegin(GL_POINTS);
		for (int i=0; i < 8; ++i) glVertex3fv(frstmPoints[i].ptr());
	glEnd();

	glLineWidth(2);
	glColor3f(0.1f, 0.5f, 0.1f); // near
	glBegin(GL_LINE_LOOP);
		for (int i=0; i < 4; ++i) glVertex3fv(frstmPoints[i].ptr());
	glEnd();
	
	glColor3f(0.1f, 0.1f, 0.5f); // far
	glBegin(GL_LINE_LOOP);
		for (int i=4; i < 8; ++i) glVertex3fv(frstmPoints[i].ptr());
	glEnd();
	
	glColor3f(0.1f, 0.5f, 0.5f);
	glBegin(GL_LINES);
		for (int i=0; i < 4; ++i) {
			glVertex3fv(frstmPoints[i].ptr()); // near
			glVertex3fv(frstmPoints[i+4].ptr()); // far
		}
	glEnd();

	glPopAttrib();
}

void DebugRenderer::setInfluenceMap(TypeMap<float> *iMap, Vec3f colour, float max) {
	imOverlay.iMap = iMap;
	imOverlay.baseColour = colour;
	imOverlay.max = max;
	influenceMap = true;
}

void DebugRenderer::renderEffects(SceneCuller &culler) {
	if (regionHilights && !rhCallback.empty()) {
		renderCellOverlay(culler, rhCallback);
	}
	if (showVisibleQuad) {
		renderCellOverlay(culler, vqCallback);
	}
	if (teamSight) {
		renderCellOverlay(culler, tsCallback);
	}
	if (HAAStarOverlay) {
		renderCellOverlay(culler, cmOverlay);
		renderPathOverlay();
		set<Vec2i>::iterator it;
		for (it = clusterEdgesWest.begin(); it != clusterEdgesWest.end(); ++it) {
			renderIntraClusterEdges(*it, CardinalDir::WEST);
		}		
		for (it = clusterEdgesNorth.begin(); it != clusterEdgesNorth.end(); ++it) {
			renderIntraClusterEdges(*it, CardinalDir::NORTH);
		}
	}
	if (resourceMapOverlay && rmOverlay.rt) {
		renderCellOverlay(culler, rmOverlay);
	}
	if (storeMapOverlay && !smOverlay.storeMaps.empty()) {
		renderCellOverlay(culler, smOverlay);
	}
	if (buildSiteMaps && !bsOverlay.cells.empty()) {
		renderCellOverlay(culler, bsOverlay);
	}
	if (showFrustum) {
		renderFrustum();
	}
	if (influenceMap) {
		renderCellOverlay(culler, imOverlay);
	}
}

DebugRenderer& getDebugRenderer() {
	static DebugRenderer debugRenderer;
	return debugRenderer;
}

}} // end namespace Glest::Debug

#endif // _GAE_DEBUG_EDITION_
