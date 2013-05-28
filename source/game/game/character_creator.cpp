
#include "pch.h"
#include "character_creator.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "util.h"
#include "FSFactory.hpp"
#include "menu_state_character_creator.h"
#include "game.h"
#include "keymap_widget.h"
#include "user_interface.h"
#include "resource_bar.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Glest::Graphics;

namespace Glest { namespace Gui {

// =====================================================
// 	class CharacterCreator
// =====================================================
CharacterCreator::CharacterCreator(CellStrip *parent, MenuStateCharacterCreator *characterCreatorMenu)
		: TabWidget(parent)
		, m_characterCreatorMenu(characterCreatorMenu) {
	setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
	// add each tab
	sov_name = "sovereign";
    sovereignState = false;

	buildSovereignTab();
	buildStatsTab();
	buildDamageTab();
	buildResourceTab();
	buildWeaponTab();
	buildArmorTab();
	buildAccessoryTab();

	if (!m_characterCreatorMenu) {
		disableWidgets();
	}
	int page = g_config.getUiLastCharacterCreatorPage();
	if (page < 0 || page > 4) {
		page = 0;
	}
	setActivePage(page);
}

CharacterCreator::~CharacterCreator() {
	g_config.setUiLastCharacterCreatorPage(getActivePage());
}

void CharacterCreator::buildSovereignTab() {
	Config &config = g_config;
	Lang &lang = g_lang;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);
	//rightPnl->addHeading(leftPnl, g_lang.get("Sovereign"));
	// Player Name
	TextBox *tb = rightPnl->addTextBox(lang.get("Sovereign Name"), "Sovereign");
	tb->TextChanged.connect(this, &CharacterCreator::onSovereignNameChanged);
	m_techTreeList = rightPnl->addDropList(lang.get("Tech Tree"));
	setupListBoxTechTree();
	m_techTreeList->SelectionChanged.connect(this, &CharacterCreator::onDropListSelectionChanged);
	m_techTreeList->setDropBoxHeight(200);
	m_focusList = rightPnl->addDropList(lang.get("Specialization"));
	setupListBoxFocus();
	m_focusList->SelectionChanged.connect(this, &CharacterCreator::onDropListSelectionChanged);
	m_focusList->setDropBoxHeight(200);

	//rightPnl->addHeading(leftPnl, g_lang.get("Free Allocation"));
    string path = "techs/" + m_techTree + "/specializations/*.";
	vector<string> paths;
	try {
		findAll(path, paths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	specializations.resize(paths.size());
	for (int i = 0; i < paths.size(); ++i) {
        string dir = "techs/" + m_techTree + "/specializations/" + paths[i] + "/" + paths[i];
        specializations[i].reset();
        if(!specializations[i].load(dir)){
        }
	}
	vector<Specialization*> listSpecs;
	for (int i = 0; i < specializations.size(); ++i) {
        listSpecs.push_back(&specializations[i]);
	}
	TabWidget::add(g_lang.get("Character"), container);
	m_spec = m_focusList->getSelectedItem()->getText();

	m_traitsList = rightPnl->addDropList(lang.get("Traits"));
	setupListBoxTraits();
	m_traitsList->SelectionChanged.connect(this, &CharacterCreator::onDropListSelectionChanged);
	m_traitsList->setDropBoxHeight(200);

	path = "techs/" + m_techTree + "/traits/*.";
	vector<string> tpaths;
	try {
		findAll(path, tpaths);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	traits.resize(tpaths.size());
	for (int i = 0; i < tpaths.size(); ++i) {
        string dir = "techs/" + m_techTree + "/traits/" + tpaths[i] + "/" + tpaths[i];
        traits[i].load(dir);
	}
    Traits traitslist;
    traitslist.resize(traits.size());
    for (int i = 0; i < traits.size(); ++i) {
        traitslist[i] = &traits[i];
    }
    m_trait = m_traitsList->getSelectedItem()->getText();
    traitsDisplay = leftPnl->addTraitsDisplay(traitslist, listSpecs, this);

	int s = g_widgetConfig.getDefaultItemHeight();
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	CellStrip *btnPanel = new CellStrip(leftPnl, Orientation::HORIZONTAL, 1);
	btnPanel->setCell(0);
	btnPanel->setAnchors(anchors);
	anchors.setCentre(true, true);
	Vec2i sz(s * 7, s);
	m_selectButton = new Button(leftPnl, Vec2i(0), sz);
	m_selectButton->setCell(0);
	m_selectButton->setPos(90, 545);
	m_selectButton->setText(lang.get("Select"));
	m_selectButton->Clicked.connect(this, &CharacterCreator::onButtonClick);

	conception = rightPnl->addSpinner("conception");
	conception->setRanges(0, 20);
	conception->setIncrement(1);
	conception->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	might = rightPnl->addSpinner("might");
	might->setRanges(0, 20);
	might->setIncrement(1);
	might->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	potency = rightPnl->addSpinner("potency");
	potency->setRanges(0, 20);
	potency->setIncrement(1);
	potency->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	spirit = rightPnl->addSpinner("spirit");
	spirit->setRanges(0, 20);
	spirit->setIncrement(1);
	spirit->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	awareness = rightPnl->addSpinner("awareness");
	awareness->setRanges(0, 20);
	awareness->setIncrement(1);
	awareness->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	acumen = rightPnl->addSpinner("acumen");
	acumen->setRanges(0, 20);
	acumen->setIncrement(1);
	acumen->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	authority = rightPnl->addSpinner("authority");
	authority->setRanges(0, 20);
	authority->setIncrement(1);
	authority->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	finesse = rightPnl->addSpinner("finesse");
	finesse->setRanges(0, 20);
	finesse->setIncrement(1);
	finesse->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	mettle = rightPnl->addSpinner("mettle");
	mettle->setRanges(0, 20);
	mettle->setIncrement(1);
	mettle->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
	fortitude = rightPnl->addSpinner("fortitude");
	fortitude->setRanges(0, 20);
	fortitude->setIncrement(1);
	fortitude->ValueChanged.connect(this, &CharacterCreator::onSpinnerValueChanged);
}

void CharacterCreator::buildResourceTab() {
	Config &config = g_config;
	Lang &lang = g_lang;
	Metrics &metrics = g_metrics;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	TabWidget::add(g_lang.get("Resources"), container);

	string load = "techs/" + m_techTree + "/provision.xml";
    XmlTree xmlTree;
    const XmlNode *itemNode;
    xmlTree.load(load);
    itemNode = xmlTree.getRootNode();

    const XmlNode *resourcesNode = itemNode->getChild("resource-types");
    resourceStats.resize(resourcesNode->getChildCount());
	for (int i = 0; i < resourcesNode->getChildCount(); ++i) {
	    const XmlNode *resourceNode = resourcesNode->getChild("resource-type", i);
        const XmlAttribute* resourceAttibute = resourceNode->getAttribute("name");
        if (resourceAttibute) {
            string craft = resourceAttibute->getRestrictedValue();
            resourceStats[i].init(0, craft);
        }
	}

    resourceSpinners.resize(resourceStats.size());
    for (int i = 0; i < resourceSpinners.size(); ++i) {
        string title = resourceStats[i].getName();
        resourceSpinners[i] = leftPnl->addSpinner(title);
        resourceSpinners[i]->setRanges(0, 20);
        resourceSpinners[i]->setIncrement(1);
        resourceSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onResourceSpinnersValueChanged);
    }
}

void CharacterCreator::buildWeaponTab() {
	Config &config = g_config;
	Lang &lang = g_lang;
	Metrics &metrics = g_metrics;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	TabWidget::add(g_lang.get("Weapons"), container);

	string load = "techs/" + m_techTree + "/provision.xml";
    XmlTree xmlTree;
    const XmlNode *itemNode;
    xmlTree.load(load);
    itemNode = xmlTree.getRootNode();

    const XmlNode *weaponsNode = itemNode->getChild("weapon-types");
    weaponStats.resize(weaponsNode->getChildCount());
	for (int i = 0; i < weaponsNode->getChildCount(); ++i) {
	    const XmlNode *weaponNode = weaponsNode->getChild("weapon-type", i);
        const XmlAttribute* weaponAttibute = weaponNode->getAttribute("name");
        if (weaponAttibute) {
            string craft = weaponAttibute->getRestrictedValue();
            weaponStats[i].init(0, craft);
        }
	}

    weaponSpinners.resize(weaponStats.size());
    for (int i = 0; i < weaponSpinners.size(); ++i) {
        string title = weaponStats[i].getName();
        weaponSpinners[i] = leftPnl->addSpinner(title);
        weaponSpinners[i]->setRanges(0, 20);
        weaponSpinners[i]->setIncrement(1);
        weaponSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onWeaponSpinnersValueChanged);
    }
}

void CharacterCreator::buildArmorTab() {
	Config &config = g_config;
	Lang &lang = g_lang;
	Metrics &metrics = g_metrics;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	TabWidget::add(g_lang.get("Armor"), container);

	string load = "techs/" + m_techTree + "/provision.xml";
    XmlTree xmlTree;
    const XmlNode *itemNode;
    xmlTree.load(load);
    itemNode = xmlTree.getRootNode();

    const XmlNode *armorsNode = itemNode->getChild("armor-types");
    armorStats.resize(armorsNode->getChildCount());
	for (int i = 0; i < armorsNode->getChildCount(); ++i) {
	    const XmlNode *armorNode = armorsNode->getChild("armor-type", i);
        const XmlAttribute* armorAttibute = armorNode->getAttribute("name");
        if (armorAttibute) {
            string craft = armorAttibute->getRestrictedValue();
            armorStats[i].init(0, craft);
        }
	}

    armorSpinners.resize(armorStats.size());
    for (int i = 0; i < armorSpinners.size(); ++i) {
        string title = armorStats[i].getName();
        armorSpinners[i] = leftPnl->addSpinner(title);
        armorSpinners[i]->setRanges(0, 20);
        armorSpinners[i]->setIncrement(1);
        armorSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onArmorSpinnersValueChanged);
    }
}

void CharacterCreator::buildAccessoryTab() {
	Config &config = g_config;
	Lang &lang = g_lang;
	Metrics &metrics = g_metrics;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	TabWidget::add(g_lang.get("Items"), container);

	string load = "techs/" + m_techTree + "/provision.xml";
    XmlTree xmlTree;
    const XmlNode *itemNode;
    xmlTree.load(load);
    itemNode = xmlTree.getRootNode();

    const XmlNode *accessoriesNode = itemNode->getChild("accessory-types");
    accessoryStats.resize(accessoriesNode->getChildCount());
	for (int i = 0; i < accessoriesNode->getChildCount(); ++i) {
	    const XmlNode *accessoryNode = accessoriesNode->getChild("accessory-type", i);
        const XmlAttribute* accessoryAttibute = accessoryNode->getAttribute("name");
        if (accessoryAttibute) {
            string craft = accessoryAttibute->getRestrictedValue();
            accessoryStats[i].init(0, craft);
        }
	}

    accessorySpinners.resize(accessoryStats.size());
    for (int i = 0; i < accessorySpinners.size(); ++i) {
        string title = accessoryStats[i].getName();
        accessorySpinners[i] = leftPnl->addSpinner(title);
        accessorySpinners[i]->setRanges(0, 20);
        accessorySpinners[i]->setIncrement(1);
        accessorySpinners[i]->ValueChanged.connect(this, &CharacterCreator::onAccessorySpinnersValueChanged);
    }
}

void CharacterCreator::buildDamageTab() {
	Config &config = g_config;
	Lang &lang = g_lang;
	Metrics &metrics = g_metrics;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(50, -1));
	container->setSizeHint(1, SizeHint(50, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	TabWidget::add(g_lang.get("Damage"), container);

	string load = "techs/" + m_techTree + "/provision.xml";
    XmlTree xmlTree;
    const XmlNode *typeNode;
    xmlTree.load(load);
    typeNode = xmlTree.getRootNode();

    leftPnl->addLabel("Damage Types");
    rightPnl->addLabel("Resistances");

    const XmlNode *damagesNode = typeNode->getChild("damage-types");
    damageTypes.resize(damagesNode->getChildCount());
    resistances.resize(damagesNode->getChildCount());
	for (int i = 0; i < damagesNode->getChildCount(); ++i) {
	    const XmlNode *damageNode = damagesNode->getChild("damage-type", i);
        const XmlAttribute* damageAttibute = damageNode->getAttribute("name");
        if (damageAttibute) {
            string damage = damageAttibute->getRestrictedValue();
            damageTypes[i].init(damage, 0);
            resistances[i].init(damage, 0);
        }
	}

    damageSpinners.resize(damageTypes.size());
    for (int i = 0; i < damageSpinners.size(); ++i) {
        string title = damageTypes[i].getTypeName();
        damageSpinners[i] = leftPnl->addSpinner(title);
        damageSpinners[i]->setRanges(0, 20);
        damageSpinners[i]->setIncrement(1);
        damageSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onDamageSpinnersValueChanged);
    }

    resistanceSpinners.resize(resistances.size());
    for (int i = 0; i < resistanceSpinners.size(); ++i) {
        string title = resistances[i].getTypeName();
        resistanceSpinners[i] = rightPnl->addSpinner(title);
        resistanceSpinners[i]->setRanges(0, 20);
        resistanceSpinners[i]->setIncrement(1);
        resistanceSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onResistanceSpinnersValueChanged);
    }
}

void CharacterCreator::buildStatsTab() {
	Config &config = g_config;
	Lang &lang = g_lang;
	Metrics &metrics = g_metrics;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	TabWidget::add(g_lang.get("Stats"), container);

    health = rightPnl->addSpinner("health");
    health->setRanges(0, 20);
    health->setIncrement(1);
    health->ValueChanged.connect(this, &CharacterCreator::onStatsSpinnersValueChanged);
    movespeed = rightPnl->addSpinner("movespeed");
    movespeed->setRanges(0, 20);
    movespeed->setIncrement(1);
    movespeed->ValueChanged.connect(this, &CharacterCreator::onStatsSpinnersValueChanged);
    attackspeed = rightPnl->addSpinner("attackspeed");
    attackspeed->setRanges(0, 20);
    attackspeed->setIncrement(1);
    attackspeed->ValueChanged.connect(this, &CharacterCreator::onStatsSpinnersValueChanged);
    sight = rightPnl->addSpinner("sight");
    sight->setRanges(0, 20);
    sight->setIncrement(1);
    sight->ValueChanged.connect(this, &CharacterCreator::onStatsSpinnersValueChanged);

	string load = "techs/" + m_techTree + "/provision.xml";
    XmlTree xmlTree;
    const XmlNode *statNode;
    xmlTree.load(load);
    statNode = xmlTree.getRootNode();

    StatGroups poolStatGroups;
    const XmlNode *poolTypesNode = statNode->getChild("pool-types");
	for (int i = 0; i < poolTypesNode->getChildCount(); ++i) {
	    const XmlNode *poolTypeNode = poolTypesNode->getChild("pool-type", i);
        const XmlAttribute* poolTypeAttibute = poolTypeNode->getAttribute("name", false);
        if (poolTypeAttibute) {
            string poolType = poolTypeAttibute->getRestrictedValue();
            StatGroup poolStatGroup;
            poolStatGroup.setName(poolType);
            poolStatGroups.push_back(poolStatGroup);
        }
	}
	enhancement.getResourcePools()->addResources(poolStatGroups);
    poolSpinners.resize(poolStatGroups.size());
    for (int i = 0; i < poolSpinners.size(); ++i) {
        string title = poolStatGroups[i].getName();
        poolSpinners[i] = rightPnl->addSpinner(title);
        poolSpinners[i]->setRanges(0, 20);
        poolSpinners[i]->setIncrement(1);
        poolSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onStatsSpinnersValueChanged);
    }

    StatGroups defenseStatGroups;
    const XmlNode *defenseTypesNode = statNode->getChild("defense-types");
	for (int i = 0; i < defenseTypesNode->getChildCount(); ++i) {
	    const XmlNode *defenseTypeNode = defenseTypesNode->getChild("defense-type", i);
        const XmlAttribute* defenseTypeAttibute = defenseTypeNode->getAttribute("name", false);
        if (defenseTypeAttibute) {
            string defenseType = defenseTypeAttibute->getRestrictedValue();
            StatGroup defenseStatGroup;
            defenseStatGroup.setName(defenseType);
            defenseStatGroups.push_back(defenseStatGroup);
        }
	}
	enhancement.getResourcePools()->addDefenses(defenseStatGroups);
    defenseSpinners.resize(defenseStatGroups.size());
    for (int i = 0; i < defenseSpinners.size(); ++i) {
        string title = defenseStatGroups[i].getName();
        defenseSpinners[i] = rightPnl->addSpinner(title);
        defenseSpinners[i]->setRanges(0, 20);
        defenseSpinners[i]->setIncrement(1);
        defenseSpinners[i]->ValueChanged.connect(this, &CharacterCreator::onStatsSpinnersValueChanged);
    }
}

void CharacterCreator::disableWidgets() {
}

void CharacterCreator::onCheckBoxChanged(Widget *src) {
}

void CharacterCreator::onSpinnerValueChanged(Widget *source) {
    if (source == conception) {
        characterStats.setConception(conception->getValue());
    } else if (source == might) {
        characterStats.setMight(might->getValue());
    } else if (source == potency) {
        characterStats.setPotency(potency->getValue());
    } else if (source == spirit) {
        characterStats.setSpirit(spirit->getValue());
    } else if (source == awareness) {
        characterStats.setAwareness(awareness->getValue());
    } else if (source == acumen) {
        characterStats.setAcumen(acumen->getValue());
    } else if (source == authority) {
        characterStats.setAuthority(authority->getValue());
    } else if (source == finesse) {
        characterStats.setFinesse(finesse->getValue());
    } else if (source == mettle) {
        characterStats.setMettle(mettle->getValue());
    } else if (source == fortitude) {
        characterStats.setFortitude(fortitude->getValue());
    }
}

void CharacterCreator::onResourceSpinnersValueChanged(Widget *source) {
    for (int i = 0; i < resourceSpinners.size(); ++i) {
        if (source == resourceSpinners[i]) {
            resourceStats[i].setValue(resourceSpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onWeaponSpinnersValueChanged(Widget *source) {
    for (int i = 0; i < weaponSpinners.size(); ++i) {
        if (source == weaponSpinners[i]) {
            weaponStats[i].setValue(weaponSpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onArmorSpinnersValueChanged(Widget *source) {
    for (int i = 0; i < armorSpinners.size(); ++i) {
        if (source == armorSpinners[i]) {
            armorStats[i].setValue(armorSpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onAccessorySpinnersValueChanged(Widget *source) {
    for (int i = 0; i < accessorySpinners.size(); ++i) {
        if (source == accessorySpinners[i]) {
            accessoryStats[i].setValue(accessorySpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onStatsSpinnersValueChanged(Widget *source) {
    if (source == health) {
        enhancement.getResourcePools()->getHealth()->getMaxStat()->setValue(health->getValue());
    }
    if (source == movespeed) {
        enhancement.getUnitStats()->getMoveSpeed()->setValue(movespeed->getValue());
    }
    if (source == attackspeed) {
        enhancement.getAttackStats()->getAttackSpeed()->setValue(attackspeed->getValue());
    }
    if (source == sight) {
        enhancement.getUnitStats()->getSight()->setValue(sight->getValue());
    }
    for (int i = 0; i < poolSpinners.size(); ++i) {
        if (source == poolSpinners[i]) {
            enhancement.getResourcePools()->getResource(i)->getMaxStat()->setValue(poolSpinners[i]->getValue());
        }
    }
    for (int i = 0; i < defenseSpinners.size(); ++i) {
        if (source == defenseSpinners[i]) {
            enhancement.getResourcePools()->getDefense(i)->getMaxStat()->setValue(defenseSpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onDamageSpinnersValueChanged(Widget *source) {
    for (int i = 0; i < damageSpinners.size(); ++i) {
        if (source == damageSpinners[i]) {
            damageTypes[i].setValue(damageSpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onResistanceSpinnersValueChanged(Widget *source) {
    for (int i = 0; i < resistanceSpinners.size(); ++i) {
        if (source == resistanceSpinners[i]) {
            resistances[i].setValue(resistanceSpinners[i]->getValue());
        }
    }
}

void CharacterCreator::onButtonClick(Widget *source) {
	if (source == m_selectButton) {
	    if (sovereignState == true) {
            for (int i = 0; i < traits.size(); ++i) {
                if (traits[i].getName() == m_trait) {
                    sovTraits.push_back(traits[i]);
                }
            }
	    } else if (sovereignState == false) {
            for (int i = 0; i < specializations.size(); ++i) {
                if (specializations[i].getSpecName() == m_spec) {
                    specialization = specializations[i];
                }
            }
	    }
	}
}

void CharacterCreator::onSovereignNameChanged(Widget *source) {
	TextBox *tb = static_cast<TextBox*>(source);
	sov_name = tb->TextWidget::getText(0);
	getSovereign()->addName(sov_name);
}

void CharacterCreator::onDropListSelectionChanged(Widget *source) {
	ListBase *list = static_cast<ListBase*>(source);
	if (list == m_focusList) {
	    m_spec = m_focusList->getSelectedItem()->getText();
	    m_trait = "-select-";
	    m_traitsList->setSelected(m_trait);
	    sovereignState = false;
	    traitsDisplay->reset();
	} else if (list == m_traitsList) {
	    m_trait = m_traitsList->getSelectedItem()->getText();
	    m_spec = "-select-";
	    m_focusList->setSelected(m_spec);
	    sovereignState = true;
	    traitsDisplay->reset();
	}
}

void CharacterCreator::onSliderValueChanged(Widget *source) {
	Slider2 *slider = static_cast<Slider2*>(source);
	if (slider == m_volFxSlider) {
		g_config.setSoundVolumeFx(slider->getValue());
		g_soundRenderer.setFxVolume(slider->getValue() / 100.f);
	}
}

void CharacterCreator::save() {
	XmlNode root("character");
	XmlNode *sovereignNode = root.addChild("sovereign");
    sovereignNode->addAttribute("name", sov_name);
    XmlNode *n;
    n = sovereignNode->addChild("specialization");
    n->addAttribute("name", specialization.getSpecName());
    n = sovereignNode->addChild("traits");
    for (int i = 0; i < sovTraits.size();++i) {
        XmlNode *traitNode = n->addChild("trait");
        traitNode->addAttribute("id", intToStr(sovTraits[i].getId()));
        traitNode->addAttribute("name", sovTraits[i].getName());
    }
    if (!characterStats.isEmpty()) {
        characterStats.save(sovereignNode->addChild("characterStats"));
    }
    n = sovereignNode->addChild("statistics");
    enhancement.save(n->addChild("enhancement"));
    XmlNode *m;
    m = n->addChild("damage-types");
	for (int i = 0; i < damageTypes.size(); ++i) {
        XmlNode *itemNode = m->addChild("damage-type");
        itemNode->addAttribute("type", damageTypes[i].getTypeName());
        itemNode->addAttribute("value", damageTypes[i].getValue());
	}
	m = n->addChild("resistances");
	for (int i = 0; i < resistances.size(); ++i) {
        XmlNode *itemNode = m->addChild("resistance");
        itemNode->addAttribute("type", resistances[i].getTypeName());
        itemNode->addAttribute("value", resistances[i].getValue());
	}
    if (!knowledge.isEmpty()) {
        knowledge.save(sovereignNode->addChild("knowledge"));
    }
    if (equipment.size() > 0) {
        n = sovereignNode->addChild("equipment");
        for (int i = 0; i < equipment.size(); ++i) {
            equipment[i].save(n->addChild("type"));
        }
    }
	n = sovereignNode->addChild("weapon-stats");
	for (int i = 0; i < weaponStats.size(); ++i) {
        XmlNode *itemNode = n->addChild("weapon");
        itemNode->addAttribute("type", weaponStats[i].getName());
        itemNode->addAttribute("value", weaponStats[i].getValue());
	}
	n = sovereignNode->addChild("armor-stats");
	for (int i = 0; i < armorStats.size(); ++i) {
        XmlNode *itemNode = n->addChild("armor");
        itemNode->addAttribute("type", armorStats[i].getName());
        itemNode->addAttribute("value", armorStats[i].getValue());
	}
	n = sovereignNode->addChild("accessory-stats");
	for (int i = 0; i < accessoryStats.size(); ++i) {
        XmlNode *itemNode = n->addChild("accessory");
        itemNode->addAttribute("type", accessoryStats[i].getName());
        itemNode->addAttribute("value", accessoryStats[i].getValue());
	}
	n = sovereignNode->addChild("resource-stats");
	for (int i = 0; i < resourceStats.size(); ++i) {
        XmlNode *resourceNode = n->addChild("resource");
        resourceNode->addAttribute("type", resourceStats[i].getName());
        resourceNode->addAttribute("value", resourceStats[i].getValue());
	}
	string fileName = "sovereigns/" + sov_name + ".xml";
	XmlIo::getInstance().save(fileName, &root);
}

void CharacterCreator::setupListBoxTechTree() {
	string techTreePath = "techs/*.";
	vector<string> techTreeFilenames;
	try {
		findAll(techTreePath, techTreeFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	m_techTreeList->addItems(techTreeFilenames);
    m_techTreeList->setSelected(techTreeFilenames[0]);
    m_techTree = techTreeFilenames[0];
}

void CharacterCreator::setupListBoxTraits() {
	string traitsPath = "techs/" + m_techTree + "/traits/*.";
	vector<string> traitsFilenames;
	try {
		findAll(traitsPath, traitsFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	vector<string> filenames;
	filenames.push_back("-select-");
	filenames.insert(filenames.end(), traitsFilenames.begin(), traitsFilenames.end());
	m_traitsList->addItems(filenames);
    m_traitsList->setSelected(filenames[0]);
    m_trait = traitsFilenames[0];
}

void CharacterCreator::setupListBoxFocus() {
	string focusPath = "techs/" + m_techTree + "/specializations/*.";
	vector<string> focusFilenames;
	try {
		findAll(focusPath, focusFilenames);
	} catch (runtime_error e) {
		g_logger.logError(e.what());
	}
	vector<string> filenames;
	filenames.push_back("-select-");
	filenames.insert(filenames.end(), focusFilenames.begin(), focusFilenames.end());
	m_focusList->addItems(filenames);
    m_focusList->setSelected(filenames[0]);
}

}}//end namespace
