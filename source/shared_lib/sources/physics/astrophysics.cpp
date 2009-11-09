// ==============================================================
//	This file is part of Glest Advanced Engine Shared Library
//
//	Copyright (C) 2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "cosmology.h"

#if defined(USE_COMPLEX_WEATHER_MODEL)

#warning this is highly incomplete code

#include <boost/foreach.hpp>
#include "xml_parser.h"

#define foreach BOOST_FOREACH

namespace Shared { namespace Physics { namespace Cosmology {


// ==============================================================
// CelestialBody
// ==============================================================

CelestialBody::CelestialBody(double mass, float meanDesntity, double meanRadius, float eccentricity,
		float meanSurfaceTemperature)
		: mass(mass)
		, meanDesntity(meanDesntity)
		, meanRadius(meanRadius)
		, eccentricity(eccentricity)
		, meanSurfaceTemperature(meanSurfaceTemperature) {
}

// ==============================================================
// Star::Classification
// ==============================================================

const Star::Classification::O("O", 30000.f);
const Star::Classification::B("B", 10000.f);
const Star::Classification::A("A", 7500.f);
const Star::Classification::F("F", 6000.f);
const Star::Classification::G("G", 5200.f);
const Star::Classification::K("K", 3700.f);
const Star::Classification::M("M", 0.f);

static const Classification &Classification::getClassification(float temperature) {
	if(!all.size()) {
		all.push_back(O);
		all.push_back(B);
		all.push_back(A);
		all.push_back(F);
		all.push_back(G);
		all.push_back(K);
		all.push_back(M);
	}

	assert(temperature >= 0.f);

	foreach(const Classification &c, all) {
		if(temperature >= c.getMinTemperature()) {
			return c;
		}
	}
	assert(0);
	return M;
}

// ==============================================================
// Orbit
// ==============================================================

Orbit::Orbit(double meanVelocity, double tilt, float eccentricity, double meanDistance,
		float relationalTilt)
		: meanVelocity(meanVelocity)
		, tilt(tilt)
		, eccentricity(eccentricity)
		, meanDistance(meanDistance)
		, relationalTilt(relationalTilt) {
}

// ==============================================================
// StarSystem
// ==============================================================

StarSystem::StarSystem(Star *primary)
		: binary(false)
		, primary(primary)
		, secondary()
		, orbit() {
}

StarSystem::StarSystem(Star *primary, Star *secondary, Orbit *orbit)
		: binary(true)
		, primary(primary)
		, secondary(secondary)
		, orbit(orbit) {
}


}}} // end namespace

#endif