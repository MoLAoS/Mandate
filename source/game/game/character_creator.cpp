
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
	buildSovereignTab();
	buildHeroTab();
	buildLeaderTab();
	buildMageTab();
	buildUnitTab();

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
	rightPnl->addHeading(leftPnl, g_lang.get("Sovereign"));
	// Player Name
	TextBox *tb = rightPnl->addTextBox(lang.get("Sovereign Name"), "Sovereign");
	tb->TextChanged.connect(this, &CharacterCreator::onSovereignNameChanged);

	// Language
	m_langList = rightPnl->addDropList(lang.get("Language"));
	setupListBoxLang();
	m_langList->SelectionChanged.connect(this, &CharacterCreator::onDropListSelectionChanged);
	m_langList->setDropBoxHeight(200);

	TabWidget::add(g_lang.get("Game"), container);
}

void CharacterCreator::buildHeroTab() {
	Config &config = g_config;
	Lang &lang = g_lang;

	CellStrip *container = new CellStrip(this, Orientation::HORIZONTAL, 2);
	container->setSizeHint(0, SizeHint(35, -1));
	container->setSizeHint(1, SizeHint(65, -1));

	OptionPanel *leftPnl = new OptionPanel(container, 0);
	OptionPanel *rightPnl = new OptionPanel(container, 1);

	rightPnl->addHeading(leftPnl, g_lang.get("Hero"));
}

void CharacterCreator::buildLeaderTab() {
	Lang &lang = g_lang;
	Config &config= g_config;

	CellStrip *panel = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP, 3);
	TabWidget::add(lang.get("Leader"), panel);
	SizeHint hint(-1, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	panel->setSizeHint(0, hint);
	panel->setSizeHint(1, hint);
	panel->setSizeHint(2, hint);

	Anchors squashAnchors(Anchor(AnchorType::SPRINGY, 10), Anchor(AnchorType::SPRINGY, 15));
}

void CharacterCreator::buildMageTab() {
	CellStrip *panel = new CellStrip(this, Orientation::VERTICAL, 2);

	StaticText *label = new StaticText(panel);
	label->setCell(0);
	label->setAnchors(Anchors::getFillAnchors());
	label->setAlignment(Alignment::FLUSH_LEFT);
	label->setText(g_lang.get("HotKeys"));
	panel->setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight() * 5 / 4));

	KeymapWidget *keymapWidget = new KeymapWidget(panel);
	keymapWidget->setCell(1);
	keymapWidget->setAnchors(Anchors::getFillAnchors());

	TabWidget::add(g_lang.get("Controls"), panel);
}

void CharacterCreator::buildUnitTab() {
	CellStrip *panel = new CellStrip(this, Orientation::HORIZONTAL, 2);
	TabWidget::add(g_lang.get("Unit"), panel);
}

void CharacterCreator::disableWidgets() {
}

void CharacterCreator::onCheckBoxCahnged(Widget *src) {
}

void CharacterCreator::onSpinnerValueChanged(Widget *source) {
}

void CharacterCreator::onSovereignNameChanged(Widget *source) {
	TextBox *tb = static_cast<TextBox*>(source);
}

void CharacterCreator::onDropListSelectionChanged(Widget *source) {
	ListBase *list = static_cast<ListBase*>(source);
	Config &config = g_config;

	if (list == m_langList) {
		map<string,string>::iterator it = m_langMap.find(list->getSelectedItem()->getText());
		string lng;
		if ( it != m_langMap.end() ) {
			lng = it->second;
		} else {
			lng = list->getSelectedItem()->getText();
		}
		config.setUiLocale(lng);
		if (m_characterCreatorMenu) {
			m_characterCreatorMenu->reload();
		} else {
			///@todo reload user_interface? - hailstone 14May2011
		}
	}
	save();
}

void CharacterCreator::onSliderValueChanged(Widget *source) {
	Slider2 *slider = static_cast<Slider2*>(source);
	if (slider == m_volFxSlider) {
		g_config.setSoundVolumeFx(slider->getValue());
		g_soundRenderer.setFxVolume(slider->getValue() / 100.f);
	}
}

// private

void CharacterCreator::save() {
	g_config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

void CharacterCreator::setupListBoxLang() {
	Config &config= Config::getInstance();

	const string langDir = "gae/data/lang/";
	vector<string> langResults;
	findAll(langDir + "*.lng", langResults, true);
	if(langResults.empty()){
        throw runtime_error("No lang files in " + langDir);
	}

	const string langListPath = langDir + "langlist.txt";
	istream *fp = FSFactory::getInstance()->getIStream(langListPath.c_str());

	// insert values into table from file (all possible lang codes)
	map<string,string> langTable;
	char buf[128];
	//while ( fgets(buf, 128, fp) ) {
	while(fp->getline(buf, 128)){
		char *code = strtok(buf, "=");
		char *lang = strtok(NULL, "=");
		if ( code && lang ) {
			langTable[string(code)] = string(lang);
		}
	}
	delete fp;

	// insert the values for langNames (the locales we care about (have lang files for))
	vector<string> langNames;
	for ( vector<string>::iterator it = langResults.begin(); it != langResults.end(); ++it ) {
		map<string,string>::iterator lcit = langTable.find(*it);
		if ( lcit != langTable.end() ) {
			if (lcit->second[lcit->second.size() - 1] == 13) {
				lcit->second[lcit->second.size() - 1] = '\0';
			}
			langNames.push_back(lcit->second);
			m_langMap[lcit->second] = *it;
		} else {
			langNames.push_back(*it);
		}
	}

	// insert values and initial value for listBoxLang
	m_langList->addItems(langNames);
	const string &loc = config.getUiLocale();
	if (langTable.find(loc) != langTable.end()) {
		m_langList->setSelected(langTable[loc]);
	} else {
		m_langList->setSelected(loc);
	}
}

// =====================================================
// 	class CharacterCreatorFrame
// =====================================================

CharacterCreatorFrame::CharacterCreatorFrame(WidgetWindow* window)
		: Frame(window, ButtonFlags::CLOSE)
		, m_saveButton(0)
		, m_characterCreator(0) {
	init();
}

CharacterCreatorFrame::CharacterCreatorFrame(Container* parent)
		: Frame(parent, ButtonFlags::CLOSE)
		, m_saveButton(0)
		, m_characterCreator(0) {
	init();
}

void CharacterCreatorFrame::init() {

	delete m_characterCreator;

	// apply/cancel buttons

	// CharacterCreator panel
	setSizeHint(1, SizeHint());
	m_characterCreatorPanel = new CellStrip(this, Orientation::HORIZONTAL, 1);
	m_characterCreatorPanel->setCell(1);
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_characterCreatorPanel->setAnchors(anchors);

	m_characterCreator = new CharacterCreator(m_characterCreatorPanel, 0);
	m_characterCreator->setCell(0);
	m_characterCreator->setAnchors(anchors);

	setDirty();
}

void CharacterCreatorFrame::init(Vec2i pos, Vec2i size, const string &title) {
	setPos(pos);
	setSize(size);
	setTitleText(title);
	layoutCells();
}

void CharacterCreatorFrame::onButtonClicked(Widget *source) {
	/*Button *btn = static_cast<Button*>(source);
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}*/
}

}}//end namespace
