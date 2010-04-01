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

#include "scene_culler.h"

#include "game_camera.h"
#include "game.h"

namespace Glest { namespace Game {

#undef max

void SceneCuller::Extrema::reset(int minY, int maxY) {
	spans.clear();
	min_y = minY;
	max_y = maxY;
	min_x = numeric_limits<int>::max();
	max_x = -1;
}

void SceneCuller::RayInfo::castRay() {
	const Map *map = World::getCurrWorld()->getMap();
	float alt = map->getAvgHeight();
	last_u = (alt - line.origin.y) / line.magnitude.y;
	if (last_u < 1.f) {
		Vec3f p = line.origin + line.magnitude * last_u;
		last_intersect = Vec2f(p.x, p.z);
		Vec2i cell((int)last_intersect.x, (int)last_intersect.y);
		if (map->isInside(cell)) {
			Vec2i lastCell;
			const int max = 3;
			int num = max;
			do {
				alt = map->getCell(cell)->getHeight();
				last_u = (alt - line.origin.y) / line.magnitude.y;
				p = line.origin + line.magnitude * (last_u < 1.f ? last_u : 1.f);
				last_intersect = Vec2f(p.x, p.z);
				lastCell = cell;
				cell = Vec2i((int)last_intersect.x, (int)last_intersect.y);
			} while (cell != lastCell && map->isInside(cell) && num--);
		}
	} else {
		// if u is > 1.f the intersection with the plane is beyond the far clip plane
		// so we 'cut off' the projection at the far clip plane, the 'y' value will be
		// wrong, but we're about to discard that anyway.
		Vec3f p = line.origin + line.magnitude * 1.f;
		last_intersect = Vec2f(p.x, p.z);
	}
}

bool SceneCuller::isInside(Vec2i pos) const {
	if (pos.y >= cellExtrema.min_y && pos.y <= cellExtrema.max_y) {
		pair<int, int> row = cellExtrema.spans[pos.y - cellExtrema.min_y];
		if (pos.x >= row.first && pos.x <= row.second) {
			return true;
		}
	}
	return false;
}


/** determine visibility of cells & tiles */
void SceneCuller::establishScene() {
	extractFrustum();
	if (getFrustumExtents()) {
		setVisibleExtrema();
	} else {
		cellExtrema.invalidate();
		tileExtrema.invalidate();
	}
}

/** Intersection of 3 planes */
Vec3f SceneCuller::intersection(const Plane &p1, const Plane &p2, const Plane &p3) const {
	float denominator = p1.n.dot(p2.n.cross(p3.n));	
	Vec3f quotient	= p2.n.cross(p3.n) * p1.d
					+ p3.n.cross(p1.n) * p2.d
					+ p1.n.cross(p2.n) * p3.d;
	return quotient / denominator;
}

/** Extract view frustum planes from projection and view matrices */
void SceneCuller::extractFrustum() {
	GLMatrix mv, proj, fm;
	glGetFloatv(GL_MODELVIEW_MATRIX, mv.raw);
	glGetFloatv(GL_PROJECTION_MATRIX, proj.raw);
	fm = proj * mv;

	frstmPlanes[Left].n.x	=	fm._41 + fm._11;
	frstmPlanes[Left].n.y	=	fm._42 + fm._12;
	frstmPlanes[Left].n.z	=	fm._43 + fm._13;
	frstmPlanes[Left].d		= -(fm._44 + fm._14);

	frstmPlanes[Right].n.x	=	fm._41 - fm._11;
	frstmPlanes[Right].n.y	=	fm._42 - fm._12;
	frstmPlanes[Right].n.z	=	fm._43 - fm._13;
	frstmPlanes[Right].d	= -(fm._44 - fm._14);

	frstmPlanes[Top].n.x	=	fm._41 - fm._21;
	frstmPlanes[Top].n.y	=	fm._42 - fm._22;
	frstmPlanes[Top].n.z	=	fm._43 - fm._23;
	frstmPlanes[Top].d		= -(fm._44 - fm._24);

	frstmPlanes[Bottom].n.x	=	fm._41 + fm._21;
	frstmPlanes[Bottom].n.y	=	fm._42 + fm._22;
	frstmPlanes[Bottom].n.z	=	fm._43 + fm._23;
	frstmPlanes[Bottom].d	= -(fm._44 + fm._24);

	frstmPlanes[Near].n.x	=	fm._41 + fm._31;
	frstmPlanes[Near].n.y	=	fm._42 + fm._32;
	frstmPlanes[Near].n.z	=	fm._43 + fm._33;
	frstmPlanes[Near].d		= -(fm._44 + fm._34);

	frstmPlanes[Far].n.x	=	fm._41 - fm._31;
	frstmPlanes[Far].n.y	=	fm._42 - fm._32;
	frstmPlanes[Far].n.z	=	fm._43 - fm._33;
	frstmPlanes[Far].d		= -(fm._44 - fm._34);

	// find near points (intersections of 'side' planes with near plane)
	frstmPoints[NearTopLeft]	 = intersection(frstmPlanes[Near], frstmPlanes[Left], frstmPlanes[Top]);
	frstmPoints[NearTopRight]	 = intersection(frstmPlanes[Near], frstmPlanes[Right], frstmPlanes[Top]);
	frstmPoints[NearBottomRight] = intersection(frstmPlanes[Near], frstmPlanes[Right], frstmPlanes[Bottom]);
	frstmPoints[NearBottomLeft]  = intersection(frstmPlanes[Near], frstmPlanes[Left], frstmPlanes[Bottom]);
	
	// far points
	frstmPoints[FarTopLeft]		= intersection(frstmPlanes[Far], frstmPlanes[Left], frstmPlanes[Top]);
	frstmPoints[FarTopRight]	= intersection(frstmPlanes[Far], frstmPlanes[Right], frstmPlanes[Top]);
	frstmPoints[FarBottomRight] = intersection(frstmPlanes[Far], frstmPlanes[Right], frstmPlanes[Bottom]);
	frstmPoints[FarBottomLeft]	= intersection(frstmPlanes[Far], frstmPlanes[Left], frstmPlanes[Bottom]);

	// normalise planes
	for (int i=0; i < 6; ++i) {
		frstmPlanes[i].normalise();
	}
}

/** project frustum edges onto a plane at avg map height & clip result to map bounds */
bool SceneCuller::getFrustumExtents() {
	const GameCamera *cam = Game::getInstance()->getGameCamera();

	Vec2f centreView(0.f);

	// collect line segments first, inserting additional interpolated ones as needed
	rays.clear();
	for (int i=0; i < 4; ++i) { // do the four from the frustum extents
		rays.push_back(RayInfo(frstmPoints[i], frstmPoints[i+4]));
		centreView += rays.back().last_intersect;
	}
	centreView /= 4.f;

	list<RayInfo>::iterator it1 = rays.begin();
	list<RayInfo>::iterator it2 = it1;
	++it2;
	for ( ; it2 != rays.end(); it1 = it2, ++it2) { 
		// check length between BL-TL, TL-TR, TR-BR, & possibly add more rays
		const float dist = it1->last_intersect.dist(it2->last_intersect);
		int newRays;
		/*if (dist > 128.f) { // insert four new rays
			newRays = 4;
		} else if (dist > 64.f) { // insert three new rays
			newRays = 3;
		} else if (dist > 32.f) { // insert two new rays
			newRays = 2;
		} else*/ if (dist > 16.f) { // insert one new ray
			newRays = 1;
		} else {	// insert zero new rays
			newRays = 0;
		}
		for (int i=0; i < newRays; ++i) {
			float frac = float(i+1) / float(newRays+1);
			rays.insert(it2, RayInfo(*it1, *it2, frac));
			//RayInfo ray(*it1, *it2, frac);
			//cout << "interpolated ray: " << ray.line.origin << ", " << ray.line.magnitude << endl;
		}
	}

	vector<Vec2f> &in = boundingPoints;
	in.clear();
	for (list<RayInfo>::iterator it = rays.begin(); it != rays.end(); ++it) {
		if (!it->valid) {
			return false;
		}
		// push them out a bit, to avoid jaggies...
		Vec2f push_dir = it->last_intersect - centreView;
		push_dir.normalize();
		push_dir *= 2;
		in.push_back(it->last_intersect + push_dir);
	}
	in.push_back(in.front()); // close poly
	clipVisibleQuad(in);
	return true;
}

/** the visit function of the line algorithm, sets cell & tile extrema as edges are evaluated */
void SceneCuller::cellVisit(int x, int y) {
	if (line_mirrored) {
		x = mirror_x + mirror_x - x;
	}
	int ty = y / 2 - tileExtrema.min_y;
	int tx = x / 2;
	y = y - cellExtrema.min_y;
	if (activeEdge == Left) {
		if (cellExtrema.spans.size() < y + 1) {
			cellExtrema.spans.push_back(pair<int,int>(x,-1));
		} else if (x < cellExtrema.spans[y].first) {
			cellExtrema.spans[y].first = x;
		}
		if (x < cellExtrema.min_x) {
			cellExtrema.min_x = x;
		}
		if (tileExtrema.spans.size() < ty + 1) {
			tileExtrema.spans.push_back(pair<int,int>(tx,-1));
		} else if ( tx < tileExtrema.spans[ty].first) {
			tileExtrema.spans[ty].first = tx;
		}
		if (tx < tileExtrema.min_x) {
			tileExtrema.min_x = tx;
		}
	} else {
		if (x > cellExtrema.spans[y].second) {
			cellExtrema.spans[y].second = x;
		}
		if (x > cellExtrema.max_x) {
			cellExtrema.max_x = x;
		}
		if (tx > tileExtrema.spans[ty].second) {
			tileExtrema.spans[ty].second = tx;
		}
		if (tx > tileExtrema.max_x) {
			tileExtrema.max_x = tx;
		}
	}
}

/** MidPoint line algorithm */
///@todo replace with Shared::Util::line<>()
void SceneCuller::scanLine(int x0, int y0, int x1, int y1) {
	assert(y0 <= y1 && x0 <= x1);
	int dx = x1 - x0,
		dy = y1 - y0;
	int x = x0,
		y = y0;

	if (dx == 0) {
		while (y <= y1) {
			cellVisit(x,y);		++y;
		}
	} else if (dy > dx) {
		int d = 2 * dx - dy;
		int incrS = 2 * dx;
		int incrSE = 2 * (dx - dy);
		do {
			cellVisit(x,y);
			if (d <= 0) {
				d += incrS;		++y;
			} else {
				d += incrSE;	++x;	++y;
			}
		} while (y <= y1);
	} else {
		int d = 2 * dy - dx;
		int incrE = 2 * dy;
		int incrSE = 2 * (dy - dx);
		do {
			cellVisit(x,y);
			if (d <= 0) {
				d += incrE;		++x;
			} else {
				d += incrSE;	++x;	++y;
			}
		} while (x <= x1);
	}
}

/** evaluate a set of edges (the left or right side), calls scanLine() for each edge */
template<typename EdgeIt>
void SceneCuller::setVisibleExtrema(const EdgeIt &start, const EdgeIt &end) {
	for (EdgeIt it = start; it != end; ++it) {
		assert(it->first.y < it->second.y);
		if (it->first.x < it->second.x)  {
			line_mirrored = false;
			scanLine(it->first.x, it->first.y, it->second.x, it->second.y);
		} else {
			line_mirrored = true;
			mirror_x = it->first.x;
			int mx = it->first.x * 2 - it->second.x;
			scanLine(it->first.x, it->first.y, mx, it->second.y);
		}
	}
}

/** sort the edges into left and right edge lists, then evaluate edge lists */
void SceneCuller::setVisibleExtrema() {
	// find indices of lowest & hightest y value
	// and therefor indices of first right edge and first left edge
	const size_t &n = visiblePoly.size();
	int ndx_min_y = -1, min_y = numeric_limits<int>::max(),
		ndx_max_y = -1, max_y = -1;
	for (int i=0; i < n; ++i) {
		Vec2i pt((int)visiblePoly[i].x, (int)visiblePoly[i].y);
		if (pt.y < min_y) {
			min_y = pt.y;
			ndx_min_y = i;
		}
		if (pt.y > max_y) {
			max_y = pt.y;
			ndx_max_y = i;
		}
	}
	// build left and right edge lists
	int i;
	vector<Edge> leftEdges, rightEdges;
	for (i = ndx_min_y; i != ndx_max_y; ++i, i %= n) {
		Vec2i pt0((int)visiblePoly[i % n].x, (int)visiblePoly[i % n].y);
		Vec2i pt1((int)visiblePoly[(i + 1) % n].x, (int)visiblePoly[(i + 1) % n].y);
		if (pt0.y < pt1.y) {
			rightEdges.push_back(Edge(pt0, pt1));
		}
	}
	for ( ; i != ndx_min_y; ++i, i %= n) {
		Vec2i pt0((int)visiblePoly[i % n].x, (int)visiblePoly[i % n].y);
		Vec2i pt1((int)visiblePoly[(i + 1) % n].x, (int)visiblePoly[(i + 1) % n].y);
		if (pt0.y > pt1.y) { // edge from pt1 to pt0 ...
			leftEdges.push_back(Edge(pt1, pt0)); // line algorithm needs first.y < second.y
		}
	}

	if (leftEdges.empty() || rightEdges.empty()) { // invalidate and bail, nothing visible
		cellExtrema.invalidate();
		tileExtrema.invalidate();
		return;
	}
	// assert sanity
	assert(leftEdges.back().first.y == rightEdges.front().first.y);
	assert(leftEdges.front().second.y == rightEdges.back().second.y);

	// set extrema
	cellExtrema.reset(min_y, max_y);
	tileExtrema.reset(min_y / 2,max_y / 2);
	activeEdge = Left;
	setVisibleExtrema(leftEdges.rbegin(), leftEdges.rend());
	activeEdge = Right;
	setVisibleExtrema(rightEdges.begin(), rightEdges.end());

	// assert sanity still prevails
	assert(cellExtrema.min_x != cellExtrema.max_x);
}

/** Liang-Barsky polygon clip.  [see Foley & Van Damn 19.1.3] */
void SceneCuller::clipVisibleQuad(vector<Vec2f> &in) {
	const float min_x = 0.f, min_y = 0.f;
	const float max_x = World::getCurrWorld()->getMap()->getW() - 2.01f;
	const float max_y = World::getCurrWorld()->getMap()->getH() - 2.01f;

	vector<Vec2f> &out = visiblePoly;

	float	xIn, yIn, xOut, yOut;
	float	tOut1, tIn2, tOut2;
	float	tInX, tOutX, tInY, tOutY;
	float	deltaX, deltaY;
	
	out.clear();

	for (unsigned i=0; i < in.size() - 1; ++i) {
		deltaX = in[i+1].x - in[i].x;
		deltaY = in[i+1].y - in[i].y;
		
		// find paramater values for x,y 'entry' points
		if (deltaX > 0 || (deltaX == 0.f && in[i].x > max_x)) {
			xIn = min_x; xOut = max_x;
		} else {
			xIn = max_x; xOut = min_x;
		}
		if (deltaY > 0 || (deltaY == 0.f && in[i].y > max_y)) {
			yIn = min_y; yOut = max_y;
		} else {
			yIn = max_y; yOut = min_y;
		}

		// find parameter values for x,y 'exit' points
		if (deltaX) {
			tOutX = (xOut - in[i].x) / deltaX;
		} else if (in[i].x <= max_x && min_x <= in[i].x ) {
			tOutX = numeric_limits<float>::infinity();
		} else {
			tOutX = -numeric_limits<float>::infinity();
		}
		if (deltaY) {
			tOutY = (yOut - in[i].y) / deltaY;
		} else if (in[i].y <= max_y && min_y <= in[i].y) {
			tOutY = numeric_limits<float>::infinity();
		} else {
			tOutY = -numeric_limits<float>::infinity();
		}
		
		if (tOutX < tOutY) {
			tOut1 = tOutX;  tOut2 = tOutY;
		} else {
			tOut1 = tOutY;  tOut2 = tOutX;
		}

		if (tOut2 > 0.f) {
			if (deltaX) {
				tInX = (xIn - in[i].x) / deltaX;
			} else {
				tInX = -numeric_limits<float>::infinity();
			}
			if (deltaY) {
				tInY = (yIn - in[i].y) / deltaY;
			} else {
				tInY = -numeric_limits<float>::infinity();
			}

			if (tInX < tInY) {
				tIn2 = tInY;
			} else {
				tIn2 = tInX;
			}

			if (tOut1 < tIn2) { // edge does not cross map
				if (0.f < tOut1 && tOut1 <= 1.f) {
					// but it does go across a corner segment, add corner...
					if (tInX < tInY) {
						out.push_back(Vec2f(xOut, yIn));
					} else {
						out.push_back(Vec2f(xIn, yOut));
					}
				}
			} else {
				if (0.f < tOut1 && tIn2 <= 1.f) {
					if (0.f < tIn2) { // edge enters map
						if (tInX > tInY) {
							out.push_back(Vec2f(xIn, in[i].y + tInX * deltaY));
						} else {
							out.push_back(Vec2f(in[i].x + tInY * deltaX, yIn));
						}
					}
					if (1.f > tOut1) { // edge exits map
						if (tOutX < tOutY) {
							out.push_back(Vec2f(xOut, in[i].y + tOutX * deltaY));
						} else {
							out.push_back(Vec2f(in[i].x + tOutY * deltaX, yOut));
						}
					} else {
						out.push_back(Vec2f(in[i+1].x, in[i+1].y));
					}
				}
			}
			// edge end point on map
			if (0.f < tOut2 && tOut2 <= 1.f) {
				out.push_back(Vec2f(xOut, yOut));
			}
		} // if (tOut2 > 0.f)
	} // for each edge
}

}}