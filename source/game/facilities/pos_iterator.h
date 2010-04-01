// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_UTIL_POSITERATOR_H_
#define _GLEST_GAME_UTIL_POSITERATOR_H_

#include <cassert>

#include "vec.h"
#include "fixed.h"
#include "math_util.h"

namespace Glest { namespace Game { namespace Util {

using Shared::Math::Vec2i;
using Shared::Math::Rect2i;
using Shared::Math::fixed;

struct PosData {
	int step;
	int off;
	fixed dist;
};

class PosCircularIterator {
protected:
	const PosData *next;
	const PosData *first;
	const PosData *last;
	int cycle;

public:
	PosCircularIterator(const PosData *next, const PosData *first, const PosData *last, int cycle)
			: next(next), first(first), last(last), cycle(cycle) {
	}

	Vec2i getPosForCycle() { 
		const int &step = next->step;
		const int &off = next->off;
		switch(cycle) {
			case 0: return Vec2i( off,   step);
			case 1: return Vec2i( step,  off);
			case 2: return Vec2i( off,  -step);
			case 3: return Vec2i(-step,  off);
			case 4: return Vec2i(-off,   step);
			case 5: return Vec2i( step, -off);
			case 6: return Vec2i(-off,  -step);
			case 7: return Vec2i(-step, -off);
			default: 
				throw runtime_error("PosCircularIterator::getPosForCycle() has bad cycle.");
		}
	}

	bool getNext(Vec2i &result) {
		assert(next <= last);
		if(next < first || cycle == (next->off ? 7 : 3)) {
			if(next++ == last) {
				return false;
			}
			cycle = 0;
		} else {
			++cycle;
		}
		result = getPosForCycle();
		return true;
	}

	bool getPrev(Vec2i &result) {
		assert(next >= first);
		if(!cycle) {
			if(next-- == first) {
				return false;
			}
			cycle = next->off ? 7 : 3;
		} else {
			--cycle;
		}
		result = getPosForCycle();
		return true;
	}

	bool getNext(Vec2i &result, fixed &dist) {
		assert(next <= last);
		if(next < first || cycle == (next->off ? 7 : 3)) {
			if(next++ == last) {
				return false;
			}
			cycle = 0;
			dist = next->dist;
		} else {
			++cycle;
		}
		result = getPosForCycle();
		return true;
	}

	bool getPrev(Vec2i &result, fixed &dist) {
		assert(next >= first);
		if(!cycle) {
			if(next-- == first) {
				return false;
			}
			cycle = next->off ? 7 : 3;
			dist = next->dist;
		} else {
			--cycle;
		}
		result = getPosForCycle();
		return true;
	}
};

class PosCircularIteratorFactory {
private:
	int maxRadius;
	size_t dataSize;
	PosData *data;
	const PosData *dataEnd;
	const PosData **radiusIndex;

private:
	PosCircularIteratorFactory(PosCircularIteratorFactory&);
	void operator=(PosCircularIteratorFactory&);

public:
	PosCircularIteratorFactory(int maxRadius);
	~PosCircularIteratorFactory();
	int getMaxRadius() const {return maxRadius;}
	int getBytesUsed() const {return dataSize * sizeof(PosData);}

	PosCircularIterator *getInsideOutIterator(int minDistance, int maxDistance) const {
		return getIterator(false, maxDistance, minDistance);
	}

	PosCircularIterator *getOutsideInIterator(int minDistance, int maxDistance) const {
		return getIterator(true, maxDistance, minDistance);
	}
	
private:
	PosCircularIterator *getIterator(bool reversed, int maxDistance, int minDistance) const;
	static int comparePosData(const void *p1, const void *p2);

	const PosData *getFirstOfDistance(int distance) const {
		assert(distance <= maxRadius);
		return radiusIndex[distance];
	}

	const PosData *getLastOfDistance(int distance) const {
		assert(distance <= maxRadius);
		const PosData *p;
		for(p = radiusIndex[distance]; p != dataEnd; ++p) {
			if(p->dist > distance) {
				break;
			}
		}
		return p - 1;
	}

	const PosData *getLessThanDistance(fixed distance) const {
		assert(distance <= maxRadius);
		const PosData *p;
		for(p = radiusIndex[distance.intp()]; p != dataEnd; ++p) {
			if(p->dist >= distance) {
				break;
			}
		}
		return p - 1;
	}

	const PosData *getGreaterThanDistance(fixed distance) const {
		assert(distance <= maxRadius);
		const PosData *p;
		for(p = radiusIndex[distance.intp()]; p != dataEnd; ++p) {
			if(p->dist > distance) {
				break;
			}
		}
		return p;
	}
};

// ===============================
// 	class PosCircularIteratorOrdered
// ===============================
/**
 * A position circular iterator who's results are garaunteed to be ordered by distance from the
 * center.  This iterator is slightly slower than PosCircularIteratorSimple, but can produce faster
 * calculations when either the nearest or furthest of a certain object is desired and the loop
 * can termainte as soon as a match is found, rather than iterating through all possibilities and
 * then calculating the nearest or furthest later.
 */
class PosCircularIteratorOrdered {
private:
	Rect2i bounds;
	Vec2i center;
	PosCircularIterator *i;

public:
	PosCircularIteratorOrdered(const Rect2i &bounds, const Vec2i &center, PosCircularIterator *i)
			: bounds(bounds), center(center), i(i)	{} 
	~PosCircularIteratorOrdered()				{ delete i; }

	bool getNext(Vec2i &result) {
		Vec2i offset;
		do {
			if(!i->getNext(offset)) {
				return false;
			}
			result = center + offset;
		} while(!bounds.isInside(result));

		return true;
	}

	bool getPrev(Vec2i &result) {
		Vec2i offset;
		do {
			if(!i->getPrev(offset)) {
				return false;
			}
			result = center + offset;
		} while(!bounds.isInside(result));

		return true;
	}

	bool getNext(Vec2i &result, fixed &dist) {
		Vec2i offset;
		do {
			if(!i->getNext(offset, dist)) {
				return false;
			}
			result = center + offset;
		} while(!bounds.isInside(result));

		return true;
	}

	bool getPrev(Vec2i &result, fixed &dist) {
		Vec2i offset;
		do {
			if(!i->getPrev(offset, dist)) {
				return false;
			}
			result = center + offset;
		} while(!bounds.isInside(result));

		return true;
	}
};
// ===============================
// 	class PosCircularIteratorSimple
// ===============================
/**
 * A position circular iterator that is more primitive and light weight than the
 * PosCircularIteratorOrdered class.  It's best used when order is not important as it uses less CPU
 * cycles for a full iteration than PosCircularIteratorOrdered (and makes code smaller).
 */
class PosCircularIteratorSimple {
private:
	Rect2i bounds;
	Vec2i center;
	int radius;
	Vec2i pos;

public:
	PosCircularIteratorSimple(const Rect2i &bounds, const Vec2i &center, int radius)
			: bounds(bounds), center(center), radius(radius), pos(center - Vec2i(radius + 1, radius)) {
	}


	bool getNext(Vec2i &result, fixed &dist) {	
		//iterate while dont find a cell that is inside the world
		//and at less or equal distance that the radius
		do {
			++pos.x;
			if (pos.x > center.x + radius) {
				pos.x = center.x - radius;
				++pos.y;
			}
			if (pos.y > center.y + radius) {
				return false;
			}
			result = pos;
			dist = fixedDist(pos, center);
		} while (dist.intp() >= (radius + 1) || !bounds.isInside(pos));
	
		return true;
	}

	bool getNext(Vec2i &result, float &dist) {	
		//iterate while dont find a cell that is inside the world
		//and at less or equal distance that the radius
		do {
			++pos.x;
			if (pos.x > center.x + radius) {
				pos.x = center.x - radius;
				++pos.y;
			}
			if (pos.y > center.y + radius) {
				return false;
			}
			result = pos;
			dist = pos.dist(center);
		} while (dist >= (radius + 1.f) || !bounds.isInside(pos));
	
		return true;
	}

	const Vec2i &getPos() {
		return pos;
	}
};


/** An iterator over a rectangular region that starts at the 'bottom-right' and proceeds right 
  * to left, bottom to top. */
class ReverseRectIterator {
private:
	int ex, wx, sy, ny;
	int cx, cy;

public:
	ReverseRectIterator(const Vec2i &p1, const Vec2i &p2) {
		if (p1.x > p2.x) {
			ex = p1.x; wx = p2.x;
		} else {
			ex = p2.x; wx = p1.x;
		}
		if (p1.y > p2.y) {
			sy = p1.y; ny = p2.y;
		} else {
			sy = p2.y; ny = p1.y;
		}
		cx = ex;
		cy = sy;
	}

	bool  more() const { return cy >= ny; }
	Vec2i next() { 
		Vec2i n(cx,cy); 
		if (cx == wx) {
			cx = ex; cy--;
		} else {
			cx--;
		}
		return n;
	}
};

}}} //last namespace

#endif // _GLEST_GAME_UTIL_POSITERATOR_H_
