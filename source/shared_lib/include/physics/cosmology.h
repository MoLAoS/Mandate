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

#ifndef _SHARED_PHYSICS_COSMOLOGY_H_
#define _SHARED_PHYSICS_COSMOLOGY_H_

/**
 * @file
 * Astromonical physics types.  Eventually, these data types will be used to feed the following:
 * - physics
 *   - gravity
 *   - wind, wind drag & weather patterns
 *   - tidal variations (effects of other celestial bodies on gravity & water levels)
 * - lighting
 *   - sky color (atmospheric coloration & star illumination taken into account)
 *   - day/night illumination (and color)
 *   - illumination from satalites (moons)
 *
 * At the moment, however, it's just a lot of shiny ideas. :)
 */

#if defined(USE_COMPLEX_WEATHER_MODEL)
#include <string>
#include <boost/shared_ptr.hpp>

#include "vec.h"
#include "patterns.h"


using std::string;
using boost::shared_ptr;
using Shared::Graphics:Vec2f;
using Shared::Graphics:Vec2d;
using Shared::Graphics:Vec3f;
using Shared::Graphics:Vec3d;
using Shared::Graphics:Vec4f;
using Shared::Graphics:Vec4d;

namespace Shared { namespace Xml {
	class XmlNode;
}}
using Shared::Xml::XmlNode;

namespace Shared { namespace Physics { namespace Cosmology {

/**
 * A base, abstract model for celetial bodies.
 */
class CelestialBody {
private:
	double mass;					/**< Total mass in kilograms (kg).  (Earth = ~5.9742e+24, Sun = ~1.9891e+30) */
	float meanDesntity;				/**< Mean desnsity of in kilograms per cubic meter (Earth = ~5515, Sun = ~1408). */
	double meanRadius;				/**< Mean radius of the celestial body in meters (m). (Earth = ~6.371e+6, Sun = ~6.96e+8)*/
	float eccentricity;				/**< How non-circular the celestial body is (i.e., flattening).  Perfectly circular = 0.  Eliptical > 0 and < 1. (Earth = ~0.0033528, Sun = ~9e-6) */
	float meanSurfaceTemperature;	/**< The mean temperature on the surface in kelvin (K). (Earth =, Sun =)*/

public:
	CelestialBody(const XmlNode &node);
	CelestialBody(double mass, float meanDesntity, double meanRadius, float eccentricity,
			float meanSurfaceTemperature);
	// allow default copy ctor & assignment operator

	double getMass() const					{return mass;}
	float getMeanDesntity() const			{return meanDesntity;}
	double getMeanRadius() const			{return meanRadius;}
	float getEccentricity() const			{return eccentricity;}
	float getMeanSurfaceTemperature() const	{return meanSurfaceTemperature;}
};

/**
 * A celestial object in a plasma state.
 */
class Star : public CelestialBody {
	/** Represents a Star's classification, which is just derived from its temperature. */
	class Classification : Uncopyable {
	private:
		string name;			/**< Classification name. */
		float minTemperature;	/**< Minimum temperature required for classification in kelvin (K) */

		static vector<const Classification> all;

		Classification(const string &name, float minTemperature)
				: name(name)
				, minTemperature(minTemperature) {
		}

	public:
		static const Classification O;
		static const Classification B;
		static const Classification A;
		static const Classification F;
		static const Classification G;
		static const Classification K;
		static const Classification M;

	public:
		const string &getName() const	{return name;}
 		float getMinTemperature() const	{return minTemperature;}

		static const Classification &getClassification(float temperature);
	};

	// fed values
	double luminosity;
	double temperature;
	double absoluteMagnitude;

	// calculated values
	Vec3d visibleRadiation;
	const Classification &classification;

public:
	Star(const XmlNode &node);
	Star(double radius, double mass, double luminosity, double temperature, double absoluteMagnitude);
	// allow default copy ctor & assignment operator
};

/**
 * Describes one side of an orbital relationship.
 */
class Orbit {
	double meanVelocity;	/**< The mean velocity of the primary object in relationship to the secondary object. */
	double tilt;			/**< The angle of axial tilt of the celestial body in radians.  (Earth =~0.409092628 (23.439281 degrees)). */
	float eccentricity;		/**< The eccentricity of the orbit. A perfect circle is zero, eliptical is > 0 and < 1. */
	double meanDistance;	/**< The mean orbital distance in meters (m) */
	double relationalTilt;	/**< Tilt of the secondary object in relationship to the orbit in radians. */

public:
	Orbit(const XmlNode &node);
	Orbit(double meanVelocity, double tilt, float eccentricity, double meanDistance,
			float relationalTilt);
	// allow default copy ctor & assignment operator

	double getMeanVelocity() const		{return meanVelocity;}
	double getTilt() const				{return tilt;}
	float getEccentricity() const		{return eccentricity;}
	double getMeanDistance() const		{return meanDistance;}
	double getEelationalTilt() const	{return relationalTilt;}
};

class StarSystem : Uncopyable {
private:
	bool binary;
	shared_ptr<Star> primary;
	shared_ptr<Star> secondary;
	shared_ptr<Orbit> orbit;

public:
	StarSystem(const XmlNode &node);
	StarSystem(Star *primary);
	StarSystem(Star *primary, Star *secondary, Orbit *orbit);

	bool isBinary() const				{return binary;}
	const Star &getPrimary() const		{return *primary;}
	const Star &getSecondary() const	{assert(isBinary()); return *secondary;}
	const Orbit &getOrbit() const		{assert(isBinary()); return *orbit;}
};


/**
 * A highly immature model for non-stellar celetial bodies (planets, moons, asteriods. etc.).
 * Eventually, this will be used to feed the base values for physics and weather calculations.  For
 * now, it's just shiny ideas. :)
 */
class NonStellarBody {
	/**
	 * Distance of the average sea-level from the core of the celesital body in kilometers.  If the
	 * celestial body has no "sea level" to reference, use the lowest point on the surface.
	 */
	double seaLevel;

	class Atmosphere {
		/**
		 * The desnsity of the atmosphere at sea level in kilograms per cubic meter at a
		 * temperature of 20 degress celcius and zero (water humidity).  This value is aproximately
		 * 1.2041 on Earth.
		 */
		double desnsity;

		/** The pressure of the atmosphere at sea level in kilopascals. (~101.3 kPa on Earth) */
		double pressure;

		/** Total dry mass in kilograms. (~5.1352e+18 on Earth) */
		double meanMass;

		/** Mean mass of vaporized hydrospheric liquid that resides in the atmposphere. (1.27e+12)*/
		double meanHydroVaporMass;

		/** How much of the visible light spectrum is absorbed by the atmosphere at sea level (i.e.,
		 * inverse of the color of the sky). */
		Vec3d visibleLightOpacity;
	};

	class Hydrosphere {
		/**
		 * The desnsity of the hydrosphere at sea level in kilograms per cubic meter at a
		 * temperature of 20 degress celcius
		 */
		double meanDesnsity;

		/** */
		double meanMass;

		/** Mean mass of disolved atmosphere that resides in the oceans (hydrosphere). */
		double meanAtmosphericMass;
	};

	float meanTemperature;
	float seasonalThermalVariance;

public:
	NonStellarBody(const XmlNode &node);
};

}}} // end namespace

#endif // USE_COMPLEX_WEATHER_MODEL
#endif // _SHARED_PHYSICS_COSMOLOGY_H_
