// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_TILESET_H_
#define _GLEST_GAME_TILESET_H_

#include <map>

#include "game_constants.h"
#include "graphics_interface.h"
#include "xml_parser.h"
#include "object_type.h"
#include "sound.h"
#include "random.h"
#include "surface_atlas.h"
#include "checksum.h"
#include "game_constants.h"
#include "renderer.h"

#include "faction_type.h"

using std::map;
using namespace Shared::Util;
using namespace Shared::Sound;
using namespace Shared::Graphics;
using Shared::Xml::XmlNode;
using Shared::Math::Vec3f;
using namespace Glest::ProtoTypes;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes { class TechTree; } }

namespace Glest { namespace Sim {
using ProtoTypes::FactionType;
using ProtoTypes::TechTree;

enum FogMode{
	fmExp,
	fmExp2
};

class Tile;

// =====================================================
//	class AmbientSounds
// =====================================================
///@todo move to Glest::Sound?
class AmbientSounds {
private:
	bool enabledDay;
	bool enabledNight;
	bool enabledRain;
	bool enabledSnow;
	bool enabledDayStart;
	bool enabledNightStart;
	bool alwaysPlayDay;
	bool alwaysPlayNight;
	StrSound day;
	StrSound night;
	StrSound rain;
	StrSound snow;
	StaticSound dayStart;
	StaticSound nightStart;

public:
	bool isEnabledDay() const			{return enabledDay;}
	bool isEnabledNight() const			{return enabledNight;}
	bool isEnabledRain() const			{return enabledRain;}
	bool isEnabledSnow() const			{return enabledSnow;}
	bool isEnabledDayStart() const		{return enabledDayStart;}
	bool isEnabledNightStart() const	{return enabledNightStart;}
	bool getAlwaysPlayDay() const		{return alwaysPlayDay;}
	bool getAlwaysPlayNight() const		{return alwaysPlayNight;}

	StrSound *getDay() 				{return &day;}
	StrSound *getNight() 			{return &night;}
	StrSound *getRain() 			{return &rain;}
	StrSound *getSnow() 			{return &snow;}
	StaticSound *getDayStart()		{return &dayStart;}
	StaticSound *getNightStart()	{return &nightStart;}

	void load(const string &dir, const XmlNode *xmlNode);
};

// =====================================================
// 	class Tileset
//
///	Containt textures, models and parameters for a tileset
// =====================================================

class Tileset {
public:
	static const int waterTexCount= 1;
	static const int surfCount= 5;
	static const int objCount= 10;

public:
	typedef vector<float> SurfProbs;
	typedef vector<Pixmap2D> SurfPixmaps;

private:
	string name;

	ObjectType objectTypes[objCount];

	SurfProbs surfProbs[surfCount];
	SurfPixmaps surfPixmaps[surfCount];

	Random random;
    Texture3D *waterTex;
    bool waterEffects;
    bool fog;
    int fogMode;
	float fogDensity;
	Vec3f fogColor;
	Vec3f sunLightColor;
	Vec3f moonLightColor;
	Gui::Weather weather;

	AmbientSounds ambientSounds;

	FactionType glestimalFactionType;

public:
	~Tileset();

	void count(const string &dir);
	void load(const string &dir, TechTree *tt);
	void doChecksum(Checksum &checksum) const;

	//get
	string getName() const							{return name;}
	ObjectType *getObjectType(int i)				{return &objectTypes[i];}
	float getSurfProb(int surf, int var) const		{return surfProbs[surf][var];}
	Texture3D *getWaterTex() const					{return waterTex;}
	bool getWaterEffects() const					{return waterEffects;}
	bool getFog() const								{return fog;}
	int getFogMode() const							{return fogMode;}
	float getFogDensity() const						{return fogDensity;}
	const Vec3f &getFogColor() const				{return fogColor;}
	const Vec3f &getSunLightColor() const			{return sunLightColor;}
	const Vec3f &getMoonLightColor() const			{return moonLightColor;}
	Gui::Weather getWeather() const					{return weather;}
	FactionType& getGlestimalFactionType()			{return glestimalFactionType;}

	//surface textures
	const Pixmap2D *getSurfPixmap(int type) const;
	const Pixmap2D *getSurfPixmap(int type, int var) const;

	//sounds
	AmbientSounds *getAmbientSounds() {return &ambientSounds;}
};

}} //end namespace

#endif
