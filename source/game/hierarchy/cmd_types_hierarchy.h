// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#ifndef _GLEST_GAME_CMDTYPESHIERARCHY_H_
#define _GLEST_GAME_CMDTYPESHIERARCHY_H_

#include "element_type.h"
#include "resource_type.h"
#include "lang.h"
#include "skill_type.h"
#include "factory.h"
#include "xml_parser.h"
#include "sound_container.h"
#include "game_constants.h"

#include "prototypes_enums.h"
#include "entities_enums.h"
#include "command_type.h"
#include "squad.h"
#include "formations.h"
#include "logger.h"

#include <set>
using std::set;

using Shared::Util::MultiFactory;
using namespace Glest::Entities;
using namespace Glest::ProtoTypes;
using Glest::Gui::Clicks;
using Glest::Search::CardinalDir;

namespace Glest { namespace Hierarchy {

// ===============================
// 	class FormationCommand
// ===============================

class FormationCommand {
public:
    Formation formation;

protected:
    Clicks     clicks;
	string     m_tipKey;
	string     m_tipHeaderKey;
	string     emptyString;

public:
    virtual Clicks getClicks() const {return clicks;}
    void describe(Formation formation, FormationDescriptor *callback) const;
	virtual void subDesc(Formation formation, FormationDescriptor *callback) const;
    const string& getTipKey() const	{return m_tipKey;}
    const string getTipKey(const string &name) const {return emptyString;}
    const string getSubHeaderKey() const {return m_tipHeaderKey;}
    void init(Formation newFormation, Clicks cl);
    void issue(Formation formation, const Squad *squad);
};

// ===============================
// 	class CreateSquadCommandType
// ===============================

class CreateSquadCommandType: public StopBaseCommandType {
private:
	const SetStructureSkillType*	m_setStructureSkillType;

public:
	CreateSquadCommandType() : StopBaseCommandType("CreateSquad", Clicks::ONE),
		m_setStructureSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;

	//get
	const SetStructureSkillType *getProductionRouteSkillType() const	{return m_setStructureSkillType;}

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::CREATE_SQUAD; }
};

// ===============================
// 	class ExpandSquadCommandType
// ===============================

class ExpandSquadCommandType: public StopBaseCommandType {
private:
	const SetStructureSkillType*	m_setStructureSkillType;

public:
	ExpandSquadCommandType() : StopBaseCommandType("ExpandSquad", Clicks::TWO),
		m_setStructureSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;

	//get
	const SetStructureSkillType *getProductionRouteSkillType() const	{return m_setStructureSkillType;}

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::EXPAND_SQUAD; }
};

// ===============================
//  class SquadMoveCommandType
// ===============================

class SquadMoveCommandType: public MoveBaseCommandType {
private:
	const MoveSkillType*	m_moveSkillType;

public:
	SquadMoveCommandType() : MoveBaseCommandType("MoveSquad", Clicks::TWO),
		m_moveSkillType(0) {}
	virtual bool load(const XmlNode *n, const string &dir, const FactionType *ft, const CreatableType *ct);
	virtual void doChecksum(Checksum &checksum) const;
	virtual void getDesc(string &str, const Unit *unit) const;
	virtual void subDesc(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt) const override;
	virtual void descSkills(const Unit *unit, CmdDescriptor *callback, ProdTypePtr pt = 0) const override;
	virtual void update(Unit *unit) const;

	//get
	const MoveSkillType *getMoveSquadSkillType() const	{return m_moveSkillType;}

	virtual CmdClass getClass() const { return typeClass(); }
	static CmdClass typeClass() { return CmdClass::MOVE_SQUAD; }
};
}}//end namespace

#endif
