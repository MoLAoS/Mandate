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

#ifndef _GLEST_GAME_AIINTERFACE_H_
#define _GLEST_GAME_AIINTERFACE_H_

#include "world.h"
#include "commander.h"
#include "command.h"
#include "conversion.h"
#include "ai.h"

using Shared::Util::intToStr;

namespace Glest{ namespace Plan {

class AiInterface {
public:
	virtual ~AiInterface() {}
	virtual void update() = 0;
};

// =====================================================
// 	class AiInterface
//
///	The AI will interact with the game through this interface
// =====================================================

class GlestAiInterface : public AiInterface {
private:
	Faction *faction;
	World *world;
	Ai ai;

	int timer;

	//config
	bool redir;
	int logLevel;

public:
	GlestAiInterface(Faction *faction, int32 randomSeed);

	//main
	void update();

	//get
	int getTimer() const		{return timer;}

	//interact
	CmdResult giveCommand(int unitIndex, CmdClass commandClass, const Vec2i &pos=Vec2i(0));
	CmdResult giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos,
		const ProducibleType* prodType);
	CmdResult giveCommand(int unitIndex, const CommandType *commandType, const Vec2i &pos);
	CmdResult giveCommand(int unitIndex, const CommandType *commandType, Unit *u= NULL);

	CmdResult giveCommand(const Unit *unit, const CommandType *commandType);
	CmdResult giveCommand(const Unit *unit, const CommandType *commandType, const Vec2i &pos,
		const ProducibleType* prodType);

	//get data
	const ControlType getControlType();
	int getMapMaxPlayers();
	Vec2i getHomeLocation();
	Vec2i getStartLocation(int locationIndex);
	int getFactionIndex() const			{ return faction->getIndex(); }
	int getFactionCount();
	int getMyUnitCount() const;
	int getMyUpgradeCount() const;
	const StoredResource *getResource(const ResourceType *rt);
	const Unit *getMyUnit(int unitIndex);
	void findEnemies(ConstUnitVector &out_list, ConstUnitPtr &out_closest);
	const FactionType *getMyFactionType();
	Faction* getFaction() { return faction; }
	Faction* getMyFaction() { return faction; }
	const TechTree *getTechTree();
	bool getNearestSightedResource(const ResourceType *rt, const Vec2i &pos, Vec2i &resultPos);
	bool isAlly(const Unit *unit) const;
	bool isAlly(int factionIndex) const;
	bool reqsOk(const RequirableType *rt);
	bool reqsOk(const CommandType *ct);
	bool checkCosts(const ProducibleType *pt);
	bool areFreeCells(const Vec2i &pos, int size, Field field);
	bool isUltra() const {return faction->getCpuUltraControl();}

private:
	string getLogFilename() const	{return "ai"+intToStr(faction->getIndex())+".log";}
};

}}//end namespace

#endif
