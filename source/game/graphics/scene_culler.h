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

#ifndef _GAME_SCENE_CULLER_INCLUDED_
#define _GAME_SCENE_CULLER_INCLUDED_

#include "vec.h"
#include "math_util.h"

#include "game_camera.h"

#include <vector>
#include <list>
#include <limits>
#include <cassert>

using namespace Shared::Math;

namespace Glest { namespace Game {

template <typename T>
inline void swap(T& a, T& b) { T temp = a; a = b; b = temp; }

struct GLMatrix {
    union {
        float raw[16]; 
		struct {
			float _11, _21, _31, _41;
			float _12, _22, _32, _42;
			float _13, _23, _33, _43;
			float _14, _24, _34, _44;
		};
    };
	
	GLMatrix(bool init=false) {
		if (init) { // init to identity
			_11 = 1.f; _21 = 0.f; _31 = 0.f; _41 = 0.f;
			_12 = 0.f; _22 = 1.f; _32 = 0.f; _42 = 0.f;
			_13 = 0.f; _23 = 0.f; _33 = 1.f; _43 = 0.f;
			_14 = 0.f; _24 = 0.f; _34 = 0.f; _44 = 1.f;
		}
	}

	bool operator==(const GLMatrix &B) {
		return !memcmp(this, &B, 4 * 4 * sizeof(float));
	}

    float& operator[] (const int ndx) { assert(ndx > -1 && ndx < 16); return raw[ndx]; }

	float& operator()(int i, int j) {
		assert(i >= 0 && i < 4 && j >= 0 && j < 4);
		return raw[j*4+i];
	}

	void transpose() {
		swap(_12, _21);
		swap(_13, _31);
		swap(_14, _41);
		swap(_23, _32);
		swap(_24, _42);
		swap(_34, _43);
	}

	GLMatrix getTranspose() const {
		GLMatrix res(*this);
		res.transpose();
		return res;
	}
	GLMatrix operator*(const GLMatrix &B) const {
		const GLMatrix &A = *this;
		GLMatrix C;
		
		C._11 = A._11 * B._11 + A._12 * B._21 + A._13 * B._31 + A._14 * B._41;
		C._12 = A._11 * B._12 + A._12 * B._22 + A._13 * B._32 + A._14 * B._42;
		C._13 = A._11 * B._13 + A._12 * B._23 + A._13 * B._33 + A._14 * B._43;
		C._14 = A._11 * B._14 + A._12 * B._24 + A._13 * B._34 + A._14 * B._44;

		C._21 = A._21 * B._11 + A._22 * B._21 + A._23 * B._31 + A._24 * B._41;
		C._22 = A._21 * B._12 + A._22 * B._22 + A._23 * B._32 + A._24 * B._42;
		C._23 = A._21 * B._13 + A._22 * B._23 + A._23 * B._33 + A._24 * B._43;
		C._24 = A._21 * B._14 + A._22 * B._24 + A._23 * B._34 + A._24 * B._44;

		C._31 = A._31 * B._11 + A._32 * B._21 + A._33 * B._31 + A._34 * B._41;
		C._32 = A._31 * B._12 + A._32 * B._22 + A._33 * B._32 + A._34 * B._42;
		C._33 = A._31 * B._13 + A._32 * B._23 + A._33 * B._33 + A._34 * B._43;
		C._34 = A._31 * B._14 + A._32 * B._24 + A._33 * B._34 + A._34 * B._44;

		C._41 = A._41 * B._11 + A._42 * B._21 + A._43 * B._31 + A._44 * B._41;
		C._42 = A._41 * B._12 + A._42 * B._22 + A._43 * B._32 + A._44 * B._42;
		C._43 = A._41 * B._13 + A._42 * B._23 + A._43 * B._33 + A._44 * B._43;
		C._44 = A._41 * B._14 + A._42 * B._24 + A._43 * B._34 + A._44 * B._44;

		return C;
	}
};

struct Plane {
	//union {
		//struct {
			Vec3f n;
			float d;
		//};
		//struct {
		//	float x, y, z, d;
		//};
		//float v[4];
	//};

	Plane() : n(0.f), d(0.f) {}
	Plane(Vec3f normal, float dist) 
		: n(normal), d(dist) {}
	void normalise() { 
		const float &len = n.length();
		n.x /= len;	n.y /= len;	n.z /= len;	d /= len;
	}
};

class DebugRenderer;

class SceneCuller {
	friend class DebugRenderer;
private:
	enum { Left, Right, Top, Bottom, Near, Far };
	Plane frstmPlanes[6];

	enum { 
		NearBottomLeft, NearTopLeft, NearTopRight, NearBottomRight,
		FarBottomLeft, FarTopLeft, FarTopRight, FarBottomRight
	};
	Vec3f frstmPoints[8];
	
	enum { TopRight, BottomRight, BottomLeft, TopLeft };

	struct Line {
		Vec3f origin, magnitude;
	};
	struct RayInfo {
		Line  line;
		float last_u;
		Vec2f last_intersect;
		bool valid;

		// construct from two points (near,far)
		RayInfo(Vec3f pt1, Vec3f pt2) {
			line.origin = pt1;
			if (pt2.y >= pt1.y) { 
				// a bit hacky, but means we don't need special case code elsewhere
				pt2.y = pt1.y - 0.1f;
			}
			line.magnitude = pt2 - pt1;
			castRay();
			if (last_u > 0.f) valid = true;
			else valid = false;
		}
		// constrcut from two other rays, interpolating line 
		RayInfo(const RayInfo &ri1, const RayInfo &ri2, const float frac) {
			line.origin = (ri1.line.origin + ri2.line.origin) * frac;
			line.magnitude = (ri1.line.magnitude + ri2.line.magnitude) * frac;
			castRay();
			if (last_u > 0.f) valid = true;
			else valid = false;
		}
		void castRay();
	};
	list<RayInfo> rays;

	vector<Vec2f> boundingPoints;
	vector<Vec2f> visiblePoly;

	typedef pair<Vec2i,Vec2i> Edge;
	static bool EdgeComp(const Edge &e0, const Edge &e1) {
		return (e0.first.y < e1.first.y);
	}

	typedef pair<int,int> RowExtrema;
	struct Extrema {
		int min_y, max_y;
		int min_x, max_x;
		vector<RowExtrema> spans;

		Rect2i getBounds() const { return Rect2i(min_x - 1, min_y - 1, max_x + 1, max_y + 1); }
		void reset(int minY, int maxY);
		void invalidate() { min_y = max_y = min_x = max_x = 0;  }

	} cellExtrema, tileExtrema;

	void extractFrustum();
	bool isPointInFrustum(const Vec3f &point) const;
	bool isSphereInFrustum(const Vec3f &centre, const float radius) const;

	Vec3f intersection(const Plane &p1, const Plane &p2, const Plane &p3) const;
	bool getFrustumExtents();
	void clipVisibleQuad(vector<Vec2f> &in);
	void setVisibleExtrema();

	template < typename EdgeIt >
	void setVisibleExtrema(const EdgeIt &start, const EdgeIt &end);

	///@todo remove, use Shared::Util::line()
	void scanLine(int x0, int y0, int x1, int y1);
	///@todo plug into line(), no need for inlining, just use adapter from functional.
	void cellVisit(int x, int y);
	
	int activeEdge;
	bool line_mirrored;
	int mirror_x;

public:
	SceneCuller()
			: activeEdge(Left)
			, line_mirrored(false)
			, mirror_x(0) {
		visiblePoly.reserve(10);
	}
	void establishScene();
	bool isInside(Vec2i pos) const;

	class iterator {
		friend class SceneCuller;
		iterator() : extrema(NULL), x(-1), y(-1) {}
		iterator(const Extrema *extrema, bool start=true) : extrema(extrema) {
			if (start) {
				x = (extrema->max_y - extrema->min_y) ? extrema->spans[0].first : -1;
				y = extrema->min_y;
			} else { // end
				x = -1;
				y = extrema->min_y + extrema->spans.size();
			}
		}
		const Extrema *extrema;
		int x, y;
		
	public:
		void operator++() {
			const int &row = y - extrema->min_y;
			if (x == extrema->spans[row].second) {
				++y;
				x = (row == extrema->spans.size() - 1 ? -1 : extrema->spans[row+1].first);
			} else {
				++x;
			}
		}
		Vec2i operator*() const { return Vec2i(x,y); }
		bool operator==(const iterator &that) const { return (x == that.x && y == that.y); }
		bool operator!=(const iterator &that) const { return !(*this == that); }
	};

	iterator cell_end_cached, tile_end_cached;

	iterator cell_begin() { 
		cell_end_cached = iterator(&cellExtrema, false);
		return iterator(&cellExtrema);
	}
	iterator cell_end()	{ 
		return cell_end_cached; 
	}

	iterator tile_begin() { 
		tile_end_cached = iterator(&tileExtrema, false);
		return iterator(&tileExtrema);
	}
	iterator tile_end() {
		return tile_end_cached;
	}

	Rect2i getBoundingRectCell() const { return cellExtrema.getBounds();	}
	Rect2i getBoundingRectTile() const { return tileExtrema.getBounds();	}
};


}}

#endif
