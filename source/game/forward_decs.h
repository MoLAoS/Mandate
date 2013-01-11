// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V2, see source/licence.txt
// ==============================================================

#ifndef _GAME_FORWARD_DECS_
#define _GAME_FORWARD_DECS_

#include "search_enums.h"
#include "prototypes_enums.h"
#include "entities_enums.h"
#include "simulation_enums.h"
#include "mandate_ai_enums.h"

namespace Glest {

	namespace Search {
		class AnnotatedMap;
		class ClusterMap;
		class NodePool;
		class NodeMap;
		class Cartographer;
		class RoutePlanner;
		class Surveyor;
	}

	namespace ProtoTypes {
		class NameIdPair;
		class DisplayableType;
		class RequirableType;
		class ProducibleType;
		class GeneratedType;
		class ResourcePools;
		class ProductionSpeeds;
		class AttackStats;
		class UnitStats;
		class Fields;
		class Zones;
		class UnitProperties;
		class EnhancementType;
		class Statistics;
		class EffectType;
		class EmanationType;
		class EffectTypeFlags;
		struct EffectTypeFlag;
		struct EffectBias;
		struct EffectStacking;
		class UnitType;
		class DamageType;
		class SkillType;
		class StopSkillType;
		class MoveSkillType;
		class TargetBasedSkillType;
		class AttackSkillType;
		class BuildSkillType;
		class HarvestSkillType;
		class TransportSkillType;
		class SetStructureSkillType;
		class RepairSkillType;
		class ProduceSkillType;
		class UpgradeSkillType;
		class BeBuiltSkillType;
		class MorphSkillType;
		class DieSkillType;
		struct AttackSkillPreference;
		class AttackSkillPreferences;
		class UpgradeType;
		class ProjectileType;
		class SplashType;
		class ResourceType;
		class MapObjectType;
		class TechTree;
		class FactionType;
		class Level;
		class ItemType;
		class CommandType;
		class AttackCommandType;
		class RepairCommandType;
		class HarvestCommandType;
		class LeaderStats;
		class Leader;
		class Mage;
		class Hero;
		class ResourceProductionSystem;
		class ItemProductionSystem;
		class ProcessProductionSystem;
		class UnitProductionSystem;
		class CreatableType;
		class Timer;
		class TimerStep;
		class CreatedItem;
		class CreatedUnit;
		class UnitsOwned;
		class Process;
	}

	namespace Sim {
		typedef int UnitId;
		class CycleInfo;
		class SimulationInterface;
		class GameSettings;
		class ParticleDamager;
		class Cell;
		class Tile;
		class Map;
		class Minimap;
		class World;
		class Tileset;
		class AmbientSounds;
		class TimeFlow;
		template<typename Type> class EntityFactory;
		template<typename Type> class SingleTypeFactory;
		template<typename Enum, typename BaseType> class DynamicTypeFactory;
		class PrototypeFactory;
	}

	namespace Entities {
		class MapObject;
		class ResourceAmount;
		class MapResource;
		class StoredResource;
        class CreatedResource; /**< added by MoLAoS, for setting automatic resource production in the XML */
		class Command;
		class Effect;
		class EffectState;
		class Effects;
		class Vec2iList;
		class UnitPath;
		class WaypointPath;
		class Unit;
		class Item;
		class Upgrade;
		class UpgradeManager;
		class Faction;
		class DirectedParticleSystem;
		class Projectile;
		class Splash;
	}

	namespace Global {
		class Config;
		class Lang;
	}

	namespace Gui {
		class GameState;
		class GameCamera;
		class Console;
		class ChatManager;
	}

	namespace Gui_Mandate {
        class TradeCommand;
        class FactionDisplay;
	}

	namespace Hierarchy {
        class Settlement;
	}

	namespace Script {
		class ScriptManager;
	}

	namespace Menu {
		class MainMenu;
		class MenuBackground;
	}

	namespace Plan {
        //class Focus;
        //class Personality;
        //class MandateAISim;
	}

}

#endif
