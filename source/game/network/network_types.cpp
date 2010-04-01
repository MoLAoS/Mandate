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

#include "pch.h"
#include "network_types.h"
#include "command.h"
#include "world.h"
#include "unit.h"

#include "leak_dumper.h"

namespace Glest { namespace Game {

// =====================================================
//	class NetworkCommand
// =====================================================

/** Construct from command with archetype == GIVE_COMMAND */
NetworkCommand::NetworkCommand(Command *command) {
	this->networkCommandType= command->getArchetype();
	this->unitId= command->getCommandedUnit()->getId();
	this->commandTypeId= command->getType()->getId();
	this->positionX= command->getPos().x;
	this->positionY= command->getPos().y;
	this->unitTypeId = command->getUnitType()
							? command->getUnitType()->getId()
							: command->getCommandedUnit()->getType()->getId();
	this->targetId = command->getUnit() ? command->getUnit()->getId() : -1;
	this->queue = command->isQueue() ? 1 : 0;
}

/** Construct archetype CANCEL_COMMAND */
NetworkCommand::NetworkCommand(NetworkCommandType type, const Unit *unit, const Vec2i &pos) {
	this->networkCommandType = type;
	this->unitId = unit->getId();
	this->commandTypeId = -1;
	this->positionX= pos.x;
	this->positionY= pos.y;
	this->unitTypeId = -1;
	this->targetId = -1;
	this->queue = 0;
}
/*
NetworkCommand::NetworkCommand(int networkCommandType, int unitId, int commandTypeId, const Vec2i &pos, int unitTypeId, int targetId){
	this->networkCommandType= networkCommandType;
	this->unitId= unitId;
	this->commandTypeId= commandTypeId;
	this->positionX= pos.x;
	this->positionY= pos.y;
	this->unitTypeId= unitTypeId;
	this->targetId= targetId;
	this->queue = 0;
}
*/
Command *NetworkCommand::toCommand() const {
	//validate unit
	World &world = World::getInstance();
	Unit* unit= world.findUnitById(unitId);
	if (!unit) {
		throw runtime_error("Can not find unit with id: " + intToStr(unitId) + ". Game out of synch.");
	}

	if (networkCommandType == NetworkCommandType::CANCEL_COMMAND) {
		return new Command(CommandArchetype::CANCEL_COMMAND, 0, Vec2i(-1), unit);
	}

	//validate command type
	const UnitType* unitType= world.findUnitTypeById(unit->getFaction()->getType(), unitTypeId);
	const CommandType* ct = unit->getType()->findCommandTypeById(commandTypeId);
	if (!ct) {
		throw runtime_error("Can not find command type with id: " + intToStr(commandTypeId) + " in unit: " + unit->getType()->getName() + ". Game out of synch.");
	}

	//get target, the target might be dead due to lag, cope with it
	Unit* target = NULL;
	if (targetId != GameConstants::invalidId) {
		target = world.findUnitById(targetId);
	}

	//create command
	Command *command= NULL;
	if (target) {
		command= new Command(ct, CommandFlags(CommandProperties::QUEUE, queue), target, unit);
	} else if(unitType){
		command= new Command(ct, CommandFlags(CommandProperties::QUEUE, queue), 
								Vec2i(positionX, positionY), unitType, unit);
	} /*else if(!target){
		command= new Command(ct, CommandFlags(), Vec2i(positionX, positionY));
	} else{
		command= new Command(ct, CommandFlags(), target);
	}*/

	return command;
}

MoveSkillUpdate::MoveSkillUpdate(const Unit *unit) {
	assert(unit->getCurrSkill()->getClass() == SkillClass::MOVE);
	Vec2i offset = unit->getNextPos() - unit->getPos();
	if (offset.length() < 1.f || offset.length() > 1.5f) {
		LOG_NETWORK( "ERROR: MoveSKillUpdate being constructed on illegal move." );
	}
	assert(offset.x >= -1 && offset.x <= 1 && offset.y >= -1 && offset.y <= 1);
	assert(offset.x || offset.y);
	assert(unit->getNextCommandUpdate() - theWorld.getFrameCount() < 256);

	this->offsetX = offset.x;
	this->offsetY = offset.y;
	this->end_offset = unit->getNextCommandUpdate() - theWorld.getFrameCount();
}

ProjectileUpdate::ProjectileUpdate(const Unit *unit, ProjectileParticleSystem *pps) {
	assert(pps->getEndFrame() - theWorld.getFrameCount() < 256);
	this->end_offset = pps->getEndFrame() - theWorld.getFrameCount();
}

#if _RECORD_GAME_STATE_

UnitStateRecord::UnitStateRecord(Unit *unit) {
	this->unit_id = unit->getId();
	this->cmd_class = unit->anyCommand() ? unit->getCurrCommand()->getType()->getClass() : -1;
	this->skill_cl = unit->getCurrSkill()->getClass();
	this->curr_pos_x = unit->getPos().x;
	this->curr_pos_y = unit->getPos().y;
	this->next_pos_x = unit->getNextPos().x;
	this->next_pos_y = unit->getNextPos().y;
	this->targ_pos_x = unit->getTargetPos().x;
	this->targ_pos_y = unit->getTargetPos().y;
	this->target_id  = unit->getTarget() ? unit->getTarget()->getId() : -1;
}

ostream& operator<<(ostream &lhs, const UnitStateRecord& state) {
	return lhs	
		<< "Unit: " << state.unit_id 
		<< ", CommandClass: " << CommandClassNames[CommandClass(state.cmd_class)]
		<< ", SkillClass: " << SkillClassNames[SkillClass(state.skill_cl)]
		<< "\n\tCurr Pos: " << Vec2i(state.curr_pos_x, state.curr_pos_y)
		<< ", Next Pos: " << Vec2i(state.next_pos_x, state.next_pos_y)
		<< "\n\tTarg Pos: " << Vec2i(state.targ_pos_x, state.targ_pos_y)
		<< ", Targ Id: " << state.target_id 
		<< endl;
}

ostream& operator<<(ostream &lhs, const FrameRecord& record) {
	if (record.frame != -1) {
		lhs	<< "Frame Record, frame number: " << record.frame << endl;
		foreach_const (FrameRecord, it, record) {
			lhs << *it;
		}
	}
	return lhs;
}

const char *gs_datafile = "game_state.gsd";
const char *gs_indexfile = "game_state.gsi";

GameStateLog::GameStateLog() {
	currFrame.frame = 0;
	// clear old data and index files
	FILE *fp = fopen(gs_datafile, "wb");
	fclose(fp);
	fp = fopen(gs_indexfile, "wb");
	fclose(fp);
}

struct StateLogIndexEntry {
	int32	frame;
	int32	start;
	int32	end;
};

void GameStateLog::writeFrame() {
	FILE *fp = fopen(gs_datafile, "ab");
	if (!fp) {
		cout << "error: cannot open game_state data file for writing: " << gs_datafile << endl;
		return;
	}
	StateLogIndexEntry ndxEntry;
	fseek(fp, 0, SEEK_END);
	ndxEntry.start = ftell(fp);
	foreach (FrameRecord, it, currFrame) {
		fwrite(&(*it), sizeof(UnitStateRecord), 1, fp);
	}
	ndxEntry.end = ftell(fp);
	fclose(fp);

	ndxEntry.frame = currFrame.frame;
	fp = fopen(gs_indexfile, "ab");
	if (!fp) {
		cout << "error: cannot open game_state index file for writing: " << gs_indexfile << endl;
		return;
	}
	fwrite(&ndxEntry, sizeof(StateLogIndexEntry), 1, fp);
	fclose(fp);
}

void GameStateLog::logFrame(int frame) {
	if (frame == -1) {
		stringstream ss;
		ss << currFrame;
		LOG_NETWORK( ss.str() );
	} else {
		assert(frame > 0);
		FILE *fp = fopen(gs_indexfile, "rb");
		if (!fp) {
			cout << "error: cannot open game_state index file for reading: " << gs_indexfile << endl;
			return;
		}
		long pos = (frame - 1) * sizeof(StateLogIndexEntry);
		StateLogIndexEntry ndxEntry;
		fseek(fp, pos, SEEK_SET);
		fread(&ndxEntry, sizeof(StateLogIndexEntry), 1, fp);
		fclose(fp);

		fp = fopen(gs_datafile, "rb");
		fseek(fp, ndxEntry.start, SEEK_SET);
		FrameRecord record;
		record.frame = frame;
		int numUpdates = (ndxEntry.end - ndxEntry.start) / sizeof(UnitStateRecord);
		assert((ndxEntry.end - ndxEntry.start) % sizeof(UnitStateRecord) == 0);
		cout << "\nEntries == " << numUpdates << endl;
			
		if (numUpdates) {
			UnitStateRecord *unitRecords = new UnitStateRecord[numUpdates];
			fread(unitRecords, sizeof(UnitStateRecord), numUpdates, fp);
			for (int i=0; i < numUpdates; ++i) {
				record.push_back(unitRecords[i]);
			}
			delete [] unitRecords;
			stringstream ss;
			ss << record;
			LOG_NETWORK( ss.str() );
		} else {
			LOG_NETWORK( "Frame " + intToStr(frame) + " has no updates." );
		}
		fclose(fp);
	}
}

#endif

}}//end namespace
