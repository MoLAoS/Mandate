// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Martiño Figueroa
//				  2010 James McCulloch <silnarm at gmail>
//				  2011 Nathan Turner <hailstone3 at sourceforge>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_CHARACTERCREATOR_H_
#define _GLEST_GAME_CHARACTERCREATOR_H_

#include "framed_widgets.h"
#include "slider.h"
#include "sigslot.h"
#include "platform_util.h"
#include "tech_tree.h"
#include "hero.h"

namespace Glest { namespace Menu {
	class MenuStateCharacterCreator;
}}

namespace Glest { namespace Gui {
using namespace Widgets;
using namespace ProtoTypes;

// =====================================================
// 	class Character Creator
//
//	Gui for creating a sovereign character
//  in the menu and game
// =====================================================
typedef vector<Specialization> ListSpec;
typedef vector<Spinner*> Spinners;
typedef vector<string> ActionNames;
class CharacterCreator : public TabWidget {
private:
    int characterCost;
    int maxCost;
    TechTree techTree;
    FactionType *factionType;
	Glest::Menu::MenuStateCharacterCreator
                           *m_characterCreatorMenu;
    CharacterStats         characterStats;
    CraftingStats          craftingStats;
    CraftStats             resourceStats;
    CraftStats             weaponStats;
    CraftStats             armorStats;
    CraftStats             accessoryStats;
    Statistics             statistics;
    EnhancementType        enhancement;
    DamageTypes            damageTypes;
    DamageTypes            resistances;
    Equipments             equipment;
    Knowledge              knowledge;
    Sovereign              sovereign;
    Specialization         *specialization;
    Traits                 sovTraits;
    Actions                actions;
    ActionNames            commandNames;
    ActionNames            skillNames;
    string                 sov_name;
    string                 m_techTree,
                           m_factionType,
                           m_spec,
                           m_trait;
	DropList	           *m_focusList,
                           *m_traitsList,
                           *m_factionTypeList,
                           *m_techTreeList;
	CheckBox	           *m_fullscreenCheckBox;
	Slider2		           *m_volFxSlider;
	Spinner                *m_minCamAltitudeSpinner;
	MessageDialog          *m_messageDialog;
	map<string,string>     m_langMap;
	TraitsDisplay          *traitsDisplay;
	SkillsDisplay          *skillsDisplay;
	Button		           *m_selectButton;
	bool                   sovereignState;
	Spinner                *conception,
                           *might,
                           *potency,
                           *spirit,
                           *awareness,
                           *acumen,
                           *authority,
                           *finesse,
                           *mettle,
                           *fortitude,
                           *movespeed,
                           *attackspeed,
                           *sight,
                           *health;
    Spinners               resourceSpinners,
                           weaponSpinners,
                           armorSpinners,
                           accessorySpinners,
                           statSpinners,
                           resistanceSpinners,
                           damageSpinners,
                           defenseSpinners,
                           poolSpinners;
public:
	CharacterCreator(CellStrip *parent, Glest::Menu::MenuStateCharacterCreator *characterCreatorMenu);
	virtual ~CharacterCreator();

    void loadTech();
    const TechTree *getTechTree() const {return &techTree;}
    TechTree *getTechTree() {return &techTree;}

    const FactionType *getFactionType() const {return factionType;}
    FactionType *getFactionType() {return factionType;}

	void save();
	virtual string descType() const override { return "CharacterCreator"; }

	string getCurrentTrait() {return m_trait;}
	string getCurrentSpec() {return m_spec;}
	Sovereign *getSovereign() {return &sovereign;}

	bool getSovereignState() {return sovereignState;}

    int getDamageTypeCount() const {return damageTypes.size();}
    int getResistanceCount() const {return resistances.size();}

    DamageType getDamageType(int i) {return damageTypes[i];}
    DamageType getResistance(int i) {return resistances[i];}

    void addActions(const Actions *addedActions, string actionName);

    void buildTabs();

    void calculateCreatorCost();

private:
	void disableWidgets();
	void setupListBoxFocus();
	void setupListBoxTraits();
	void setupListBoxTechTree();
	void setupListBoxFactionType();
	void onButtonClick(Widget *source);

	// Build tabs
	void buildSovereignTab();
	void buildSkillsTab();
	void buildStatsTab();
	void buildDamageTab();
	void buildResourceTab();
	void buildWeaponTab();
	void buildArmorTab();
	void buildAccessoryTab();

	// Event callbacks
	void onCheckBoxChanged(Widget *source);
	void onSliderValueChanged(Widget *source);
	void onSpinnerValueChanged(Widget *source);
	void onWeaponSpinnersValueChanged(Widget *source);
	void onArmorSpinnersValueChanged(Widget *source);
	void onAccessorySpinnersValueChanged(Widget *source);
	void onResourceSpinnersValueChanged(Widget *source);
	void onStatsSpinnersValueChanged(Widget *source);
	void onDamageSpinnersValueChanged(Widget *source);
	void onResistanceSpinnersValueChanged(Widget *source);
	void onDropListSelectionChanged(Widget *source);
	void onSovereignNameChanged(Widget *source);
};

}}//end namespace

#endif
