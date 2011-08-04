// ==============================================================
//	This file is part of the Glest Advanced Engine
//
//	Copyright (C) 2010 James McCulloch
//
//	GPL V2. see source/liscence.txt
// ==============================================================

#ifndef _GLEST_GAME_CLOAKTYPE_H_
#define _GLEST_GAME_CLOAKTYPE_H_

#include "element_type.h"
#include "sound_container.h"
#include "checksum.h"
#include "unit_stats_base.h"
#include "prototypes_enums.h"
#include "influence_map.h"
#include "shader.h"
#include <set>
using std::set;

namespace Glest { namespace ProtoTypes {

using namespace Search;
using Shared::Sound::StaticSound;
using Shared::Util::Checksum;
using Shared::Util::MultiFactory;
using Shared::Graphics::ShaderProgram;

// =====================================================
//  class CloakType
// =====================================================

class CloakType : public RequirableType {
	
	friend class Sim::SingleTypeFactory<CloakType>;

private:
	const UnitType   *m_unitType;
	CloakClass        m_class;
	int32             m_group;
	int32             m_cost;
	StaticSound      *m_cloakSound;
	StaticSound      *m_deCloakSound;
	/*Texture2D        *m_image;*/
	ShaderProgram    *m_allyShader;
	ShaderProgram    *m_enemyShader;

private:
	CloakType(const UnitType *ut);

public:
	virtual ~CloakType();

	void load(const string &dir, const XmlNode *xmlNode, const TechTree *tt,
		vector<string> &out_decloakSkillNames, vector<SkillClass> &out_decloakSkillClasses);

	const UnitType* getUnitType() const    { return m_unitType; }
	CloakClass getClass() const            { return m_class; }
	bool isCloakGroup(int group) const     { return m_group == group; }
	int  getCloakGroup() const             { return m_group; }
	int  getEnergyCost() const             { return m_cost; }
	StaticSound* getCloakSound() const     { return m_cloakSound; }
	StaticSound* getDeCloakSound() const   { return m_deCloakSound; }
	ShaderProgram* getAllyShader() const   { return m_allyShader; }
	ShaderProgram* getEnemyShader() const  { return m_enemyShader; }

	void doChecksum(Checksum &cs) const;
};

// =====================================================
//  class DetectorType
// =====================================================

class DetectorType : public RequirableType {
	friend class Sim::SingleTypeFactory<DetectorType>;

public:
	typedef vector<int>  Groups;

private:
	const UnitType   *m_unitType;
	DetectorClass     m_class;
	Groups            m_groups;
	int               m_cost;
	StaticSound      *m_detectionOnSound;
	StaticSound      *m_detectionOffSound;
	StaticSound      *m_detectionMadeSound;

private:
	DetectorType(const UnitType *ut);

public:
	virtual ~DetectorType();

	void load(const string &dir, const XmlNode *xmlNode, const TechTree *tt);

	const UnitType* getUnitType() const    { return m_unitType; }
	DetectorClass getClass() const         { return m_class; }
	int  getEnergyCost() const             { return m_cost; }
	bool detectsCloakGroup(int group) const;
	int  getGroupCount() const             { return m_groups.size(); }
	int  getGroup(int i) const             { return m_groups[i]; }
	//Groups::const_iterator groups_begin() const  { return m_groups.begin(); }
	//Groups::const_iterator groups_end() const    { return m_groups.end();   }

	StaticSound* getDetectOnSound() const  { return m_detectionOnSound; }
	StaticSound* getDetectffSound() const  { return m_detectionOffSound; }
	StaticSound* getDetectSound() const    { return m_detectionMadeSound; }

	void doChecksum(Checksum &cs) const;
};

}}

#endif
