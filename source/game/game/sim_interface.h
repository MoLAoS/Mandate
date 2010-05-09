// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  Based on code Copyright (C) 2001-2008 Martiño Figueroa
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#ifndef _GAME_SIMULATION_INTERFACE_H_
#define _GAME_SIMULATION_INTERFACE_H_

#include "game.h"
#include "world.h"
#include "element_type.h"
#include "game_settings.h"
#include "config.h"
#include "ai_interface.h"
#include "network_types.h"
#include "network_message.h"
#include "stats.h"
#include "FSFactory.hpp"
#include "util.h"

using Shared::Graphics::ParticleSystem;

namespace Glest { namespace Net {
	class NetworkInterface;
	class ClientInterface;
	class ServerInterface;
}}
using namespace Glest::Net;

namespace Glest {

#if _GAE_DEBUG_EDITION_

namespace Debug {

struct UnitStateRecord {

	uint32	unit_id		: 32;
	int32	cmd_id		: 16;
	uint32	skill_id	: 16; // 8
	int32	curr_pos_x	: 12;
	int32	curr_pos_y	: 12;
	int32	next_pos_x	: 12;
	int32	next_pos_y	: 12; // 16
	int32	targ_pos_x	: 12;
	int32	targ_pos_y	: 12;
	int32	target_id	: 32; // 24

	UnitStateRecord(Unit *unit);
	UnitStateRecord() {}

};					//	: 24 bytes

ostream& operator<<(ostream &lhs, const UnitStateRecord&);

struct FrameRecord : public vector<UnitStateRecord> {
	int32	frame;
};

ostream& operator<<(ostream &lhs, const FrameRecord&);

class WorldLog {
	FrameRecord currFrame;

	void writeFrame();

	FileOps *dataFile, *indexFile;

public:
	WorldLog();
	~WorldLog();

	int getCurrFrame() const { return currFrame.frame; }

	void addUnitRecord(UnitStateRecord &usr) {
		currFrame.push_back(usr);
	}

	void newFrame(int frame) {
		if (currFrame.frame) {
			writeFrame();
		}
		currFrame.clear();
		assert(frame == currFrame.frame + 1);
		currFrame.frame = frame;
	}

	void logFrame(int frame = -1);
};

} // namespace Debug

using namespace Debug;

#endif // _GAE_DEBUG_EDITION_

namespace Sim {

WRAPPED_ENUM( QuitSource, LOCAL, SERVER )
WRAPPED_ENUM( GameStatus, NO_CHANGE, LOST, WON )

class CycleInfo {
	int skillFrames, animFrames;
	int soundOffset, attackOffset;

public:
	CycleInfo()
			: skillFrames(-1)
			, animFrames(-1)
			, soundOffset(-1)
			, attackOffset(-1) {
	}

	CycleInfo(int sFrames, int aFrames, int sOffset = -1, int aOffset = -1)
			: skillFrames(sFrames)
			, animFrames(aFrames)
			, soundOffset(sOffset)
			, attackOffset(aOffset) {
	}

	int getSkillFrames() const	{ return skillFrames;	}
	int getAnimFrames() const	{ return animFrames;	}
	int getSoundOffset() const	{ return soundOffset;	}
	int getAttackOffset() const { return attackOffset;	}
};

class SkillCycleTable : public Net::Message {
private:
	MsgHeader header;
	CycleInfo *cycleTable;
	int numEntries;

public:
	SkillCycleTable() : cycleTable(0) {
		header.messageType = MessageType::SKILL_CYCLE_TABLE;
	}
	SkillCycleTable(RawMessage raw);

	void create(const TechTree *techTree);

	const CycleInfo& lookUp(int skillId) { return cycleTable[skillId]; }
	const CycleInfo& lookUp(const Unit *unit) { return cycleTable[unit->getCurrSkill()->getId()]; }

	int size() const { return numEntries; }

	virtual bool receive(NetworkConnection* connection);
	virtual void send(NetworkConnection* connection) const;
	virtual void send(Socket* socket) const { throw runtime_error("you should implement "__FUNCTION__); }
};

class SimulationInterface {
public:
	typedef vector<Plan::AiInterface*> AiInterfaces;
	typedef vector<NetworkCommand> Commands;

protected:
	Program &program;
	GameState *game;
	World *world;
	Stats *stats;

	UnitMap units;

	GameSettings gameSettings;
	XmlNode *savedGame;

	AiInterfaces aiInterfaces;
	Commander *commander;

	UnitFactory unitFactory;

	bool paused;
	bool gameOver;
	bool quit;
	GameSpeed speed, prevSpeed;

	Commands requestedCommands;	//commands requested by the user
	Commands pendingCommands;	//commands ready to be given

	SkillCycleTable skillCycleTable;

#	if _GAE_DEBUG_EDITION_
		WorldLog worldLog;
#	endif

public:
	SimulationInterface(Program &program);
	SimulationInterface(const SimulationInterface &si);
	virtual ~SimulationInterface();

	int getUpdateInterval() const;

	UnitFactory& getUnitFactory()			{ return unitFactory; }
	GameSettings &getGameSettings()			{ return gameSettings; }
	XmlNode* getSavedGame() const			{ return savedGame; }
	Commander *getCommander()				{ return commander; }
	const Commander *getCommander() const	{ return commander; }
	GameState* getGameState()				{ return game; }
	const GameState* getGameState() const	{ return game; }
	World *getWorld()						{ return world; }
	const World *getWorld() const			{ return world; }
	Stats* getStats()						{ return stats; }
	bool getQuit() const					{ return quit; }

	// create/destroy World
	void constructGameWorld(GameState *g);
	void destroyGameWorld();

	// load/init/update
	void loadWorld();
	void initWorld();
	int launchGame();
	void updateWorld();

	// game speed
	GameSpeed pause();
	GameSpeed resume();
	GameSpeed incSpeed();
	GameSpeed decSpeed();
	GameSpeed resetSpeed();
	GameSpeed getSpeed() const { return speed; }
	int getUpdateLoops();

	// game over checks
	GameStatus checkWinner();
	GameStatus checkWinnerStandard();
	GameStatus checkWinnerScripted();
	bool hasBuilding(const Faction *faction);

	// world interface [all for networking]
	void startFrame(int frame);
	void doUnitBorn(Unit *unit);
	void doUpdateProjectile(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end);
	void doUpdateUnitCommand(Unit *unit);
	void doUpdateAnim(Unit *unit);

	void doQuitGame(QuitSource source);

	// commands
	virtual void requestCommand(const NetworkCommand *networkCommand) {
		requestedCommands.push_back(*networkCommand);
	}
	virtual void requestCommand(Command *command);

	// change interface type
	void changeRole(GameRole role);

	// query interface type
	virtual GameRole getNetworkRole() const { return GameRole::LOCAL; }
	bool isNetworkInterface() const { return getNetworkRole() != GameRole::LOCAL; }

	// retrieve derived type, or NULL (ie, calling is always 'safe', but check the return for NULL)
	NetworkInterface*	asNetworkInterface();
	ClientInterface*	asClientInterface();
	ServerInterface*	asServerInterface();

protected:
	// game life-cycle

	/** Create, create & send or receive the SkillCycleTable */
	virtual void createSkillCycleTable(const TechTree *techTree) {
		skillCycleTable.create(techTree);
	}

	/** Create & Synchronise AI random number seeds */
	virtual void syncAiSeeds(int aiCount, int *seeds) {
		Random r;
		r.init(Chrono::getCurMillis());
		for (int i=0; i < aiCount; ++i) {
			seeds[i] = r.rand();
		}
	}

	/** Wait on network until all players are ready to start the game, only for network games */
	virtual void waitUntilReady(Checksum*)  { }

	/** Indicator that the game should now start, only used by ClientInterface */
	virtual void startGame() { }

	/** Called after each world frame is processed, issues pending commands */
	virtual void frameProccessed() {
		std::copy(requestedCommands.begin(), requestedCommands.end(), std::back_inserter(pendingCommands));
		requestedCommands.clear();
	}

	/** Called when a quit request is received */
	virtual void quitGame(QuitSource) { }

	/** Called after a command has been updated, determines skill cycle length */
	virtual void updateSkillCycle(Unit*);

	/** Calculates and sets 'arrival time' for a new projectile */
	virtual void updateProjectilePath(Unit *u, Projectile *pps, const Vec3f &start, const Vec3f &end) {
		pps->setPath(start, end);
	}

	// post event hooks, The following four functions are used by NetworkInterfaces to collect and compare
	// checksums of all interesting events... once network play has stabalised, they should either
	// be removed or compiled into only the debug ed.

	/** Called after a command update, after the skill cycle length is set */
	virtual void postCommandUpdate(Unit *unit) {}

	/** Called after a projectile's 'path' is set */
	virtual void postProjectileUpdate(Unit *unit, int endFrame) {}

	/** Called after a unit's animatioin cycle is updated */
	virtual void postAnimUpdate(Unit *unit) {}

	/** Called afer a unit is born */
	virtual void postUnitBorn(Unit *unit) {}
};

}}

#endif
