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
		class AttackType;
		class ArmourType;
		class DamageMultiplierTable;
		class UnitStats;
		class Fields;
		class Zones;
		class UnitProperties;
		class EnhancementType;
		class EffectType;
		class EmanationType;
		class EffectTypeFlags;
		struct EffectTypeFlag;
		struct EffectBias;
		struct EffectStacking;
		class UnitType;
		class SkillType;
		class StopSkillType;
		class MoveSkillType;
		class TargetBasedSkillType;
		class AttackSkillType;
		class BuildSkillType;
		class HarvestSkillType;
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
		class CommandType;
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
		class Command;
		class Effect;
		class EffectState;
		class Effects;
		class Vec2iList;
		class UnitPath;
		class WaypointPath;
		class Unit;
		class Upgrade;
		class UpgradeManager;
		class Faction;
		class AttackParticleSystem;
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

	namespace Script {
		class ScriptManager; 
	}

	namespace Menu {
		class MainMenu;
		class MenuBackground;
	}

}

#endif
