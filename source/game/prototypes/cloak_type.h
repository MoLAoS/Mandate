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
	uint32            m_tags;
	int32             m_cost;
	StaticSound      *m_cloakSound;
	StaticSound      *m_deCloakSound;
	/*Texture2D        *m_image;*/
	UnitShaderSet    *m_allyShaders;
	UnitShaderSet    *m_enemyShaders;

private:
	CloakType(const UnitType *ut);

public:
	virtual ~CloakType();

	void load(const string &dir, const XmlNode *xmlNode, const TechTree *tt,
		vector<string> &out_decloakSkillNames, vector<SkillClass> &out_decloakSkillClasses);

	const UnitType* getUnitType() const    { return m_unitType; }
	CloakClass getClass() const            { return m_class; }
	bool isTagType(int tag) const          { assert(tag); return (1 << tag) & m_tags; }
	int getEnergyCost() const              { return m_cost; }
	StaticSound* getCloakSound() const     { return m_cloakSound; }
	StaticSound* getDeCloakSound() const   { return m_deCloakSound; }
	/*const Texture2D* getImage() const      { return m_image; }*/
	UnitShaderSet* getAllyShaders() const  { return m_allyShaders; }
	UnitShaderSet* getEnemyShaders() const { return m_enemyShaders; }

	void doChecksum(Checksum &cs) const;
};

class DetectorType : public RequirableType {
private:
	bool	m_detector;
	///@todo ... lots ...
	//DetectorClass     m_class;
	//uint32            m_tags;
	//etc...
};

}}

#endif
