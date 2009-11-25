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

namespace Glest { namespace Game { namespace Util {

using Shared::Graphics::Vec2i;

struct PosData {
	int step;
	int off;
	float dist;
};

class PosCircularIterator {
protected:
	const PosData *next;
	const PosData *first;
	const PosData *last;
	int cycle;

public:
	PosCircularIterator(const PosData *next, const PosData *first, const PosData *last, int cycle) :
			next(next),
			first(first),
			last(last),
			cycle(cycle) {
	}

	Vec2i getPosForCycle() { 
		int step = next->step;
		int off = next->off;
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
				assert(false); 
				return Vec2i(0);		
		}
	}
/*
	Vec2i getPosForCycle() { 
		int step = cycle & 2 ? -next->step : next->step;
		int off = cycle & 4 ? -next->off : next->off;
		return cycle & 1 ? Vec2i(step, off) :  Vec2i(off, step);
	}
*/
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

	bool getNext(Vec2i &result, float &dist) {
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

	bool getPrev(Vec2i &result, float &dist) {
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

	const PosData *getLessThanDistance(float distance) const {
		assert(distance <= maxRadius);
		const PosData *p;
		for(p = radiusIndex[static_cast<int>(distance)]; p != dataEnd; ++p) {
			if(p->dist >= distance) {
				break;
			}
		}
		return p - 1;
	}

	const PosData *getGreaterThanDistance(float distance) const {
		assert(distance <= maxRadius);
		const PosData *p;
		for(p = radiusIndex[static_cast<int>(distance)]; p != dataEnd; ++p) {
			if(p->dist > distance) {
				break;
			}
		}
		return p;
	}
};

}}} //last namespace

#endif // _GLEST_GAME_UTIL_POSITERATOR_H_
