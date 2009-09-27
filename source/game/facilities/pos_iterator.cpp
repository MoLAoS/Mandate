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

#include "pch.h"
#include "pos_iterator.h"

#include "leak_dumper.h"

namespace Glest { namespace Game { namespace Util {

PosCircularIteratorFactory::PosCircularIteratorFactory(int maxRadius) :
		maxRadius(maxRadius),
		// I promise this calculation is correct and I'm sorry that it looks ugly :)
		dataSize(((maxRadius + 1) / 2) * ((maxRadius & 0xfffffffe) + 1)),
		data(NULL),
		dataEnd(NULL),
		radiusIndex(NULL) {
	assert(maxRadius > 0);
	data = new PosData[dataSize];
	dataEnd = data + dataSize;
	radiusIndex = new const PosData*[maxRadius + 1];
	
	// Calculate distance data
	PosData *p = data;
	for(int step = 0; step < maxRadius; ++step) {
		for(int off = 0; off <= step; ++off) {
			p->step = step;
			p->off = off;
			p->dist = sqrtf(static_cast<float>(step * step + off * off));
			++p;
		}
	}
	assert(p - data == dataSize);
	assert(p == dataEnd);
	assert(dataEnd == &data[dataSize]);
	
	// Sort data by distance
	qsort(data, dataSize, sizeof(PosData), PosCircularIteratorFactory::comparePosData);

	// Calculate radius index
	int nextRadius = 0;
	for(p = data; p != dataEnd && nextRadius < maxRadius; ++p) {
		if(p->dist >= nextRadius) {
			radiusIndex[nextRadius++] = p;
		}
	}
	assert(nextRadius == maxRadius);	// Make sure we are sane (data came out ok)
	assert(radiusIndex[0] = data);		// zero radius should be 1st data element
	// the last entry in radiusIndex should point one item beyond data, but should never be
	// dereferenced, like the end() of an stl collection
	radiusIndex[maxRadius] = dataEnd;	
}

PosCircularIteratorFactory::~PosCircularIteratorFactory() {
	delete[] data;
	delete[] radiusIndex;
}

int PosCircularIteratorFactory::comparePosData(const void *p1, const void *p2) {
	const PosData &pd1 = *reinterpret_cast<const PosData *>(p1);
	const PosData &pd2 = *reinterpret_cast<const PosData *>(p2);
	return pd1.dist == pd1.dist ? 0 : (pd1.dist > pd2.dist ? 1 : -1);
}

PosCircularIterator *PosCircularIteratorFactory::getIterator(bool reversed, int maxDistance, int minDistance) const {
	assert(minDistance >= 0);
	assert(maxDistance >= minDistance);
	assert(maxDistance <= maxRadius);
	if(minDistance > maxRadius) {
		minDistance = maxRadius;
	}
	if(maxDistance > maxRadius) {
		maxDistance = maxRadius;
	}
	const PosData *first = getFirstOfDistance(minDistance);
	const PosData *last = getLastOfDistance(maxDistance);
	return reversed 
			? new PosCircularIterator(last + 1, first, last, 0)
			: new PosCircularIterator(first - 1, first, last, 7);
}

}}}//end namespace

