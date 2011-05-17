
#include "pch.h"
#include "options.h"

#include "renderer.h"
#include "sound_renderer.h"
#include "core_data.h"
#include "config.h"
#include "util.h"
#include "FSFactory.hpp"
#include "menu_state_options.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Glest::Graphics;

namespace Glest { namespace Gui {

// =====================================================
// 	class RelatedDoubleOption
// =====================================================

class RelatedDoubleOption : public CellStrip {
public:
	RelatedDoubleOption(Container *parent, const string &title, const string &txt1, const string &txt2)
			: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 5) {
		setSizeHint(0, SizeHint(40));
		setSizeHint(1, SizeHint(10));
		setSizeHint(2, SizeHint(20));
		setSizeHint(3, SizeHint(10));
		setSizeHint(4, SizeHint(20));

		Anchors dwAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2),
			Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2));
		setAnchors(dwAnchors);

		StaticText *label = new StaticText(this);
		label->setText(title);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(0);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);

		label = new StaticText(this);
		label->setText(txt1);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(1);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);

		label = new StaticText(this);
		label->setText(txt2);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(3);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);
	}

	void setCustomSplits(int label, int val1, int val2) {
		setSizeHint(1, SizeHint(label));
		setSizeHint(2, SizeHint(val1));
		setSizeHint(3, SizeHint(label));
		setSizeHint(4, SizeHint(val2));
	}
};

// =====================================================
// 	class CheckBoxHolder
// =====================================================

class CheckBoxHolder : public CellStrip {
public:
	CheckBoxHolder(Container *parent)
			: CellStrip(parent, Orientation::HORIZONTAL, 3) {
		setSizeHint(0, SizeHint());
		setSizeHint(1, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
		setSizeHint(2, SizeHint());
		setAnchors(Anchors(Anchor(AnchorType::RIGID, 0)));
	}
};

// =====================================================
// 	class Spinner
// =====================================================

Spinner::Spinner(Container *parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 2)
		, m_minValue(0), m_maxValue(0), m_increment(1), m_value(0) {
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	setAnchors(anchors);
	m_valueBox = new SpinnerValueBox(this);
	m_valueBox->setCell(0);
	m_valueBox->setAnchors(anchors);
	m_valueBox->setText("0");

	setSizeHint(1, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
	m_upButton = new ScrollBarButton(this, Direction::UP);
	m_upButton->setCell(1);
	Anchors a = anchors;
	a.set(Edge::BOTTOM, 50, true);
	m_upButton->setAnchors(a);
	m_upButton->Fire.connect(this, &Spinner::onButtonFired);

	m_downButton = new ScrollBarButton(this, Direction::DOWN);
	m_downButton->setCell(1);
	a = anchors;
	a.set(Edge::TOP, 50, true);
	m_downButton->setAnchors(a);
	m_downButton->Fire.connect(this, &Spinner::onButtonFired);
}

void Spinner::onButtonFired(Widget *source) {
	ScrollBarButton *btn = static_cast<ScrollBarButton*>(source);
	int val = m_value + (btn == m_upButton ? m_increment : -m_increment);
	val = clamp(val, m_minValue, m_maxValue);
	if (val != m_value) {
		if (m_value == m_minValue) {
			m_downButton->setEnabled(true);
		}
		if (m_value == m_maxValue) {
			m_upButton->setEnabled(true);
		}
		m_value = val;
		m_valueBox->setText(intToStr(m_value));
		ValueChanged(this);
		if (m_value == m_minValue) {
			m_downButton->setEnabled(false);
		}
		if (m_value == m_maxValue) {
			m_upButton->setEnabled(false);
		}
	}
}

// =====================================================
// 	class Options
// =====================================================

Options::Options(CellStrip *parent, MenuStateOptions *optionsMenu)
		: TabWidget(parent)
		, m_optionsMenu(optionsMenu) {

	// add each tab
	//addTab(Button*, CellStrip*)
	//Game
	//Video
	//Audio
	//Network
	//Controls
	//Debug

	buildOptionsPanel(parent, 0);

	if (!m_optionsMenu) {
		disableWidgets();
	}
}

void Options::disableWidgets() {
	// disable all the widgets until they're known to be ready for ingame - hailstone 14May2011
	m_langList->setEnabled(false);
	m_shadowsList->setEnabled(false);
	m_filterList->setEnabled(false);
	m_lightsList->setEnabled(false);
	m_modelShaderList->setEnabled(false);

	m_3dTexCheckBox->setEnabled(false);
	m_debugModeCheckBox->setEnabled(false);
	m_debugKeysCheckBox->setEnabled(false);
	
	//m_volFxSlider->setEnabled(false);
	//m_volAmbientSlider->setEnabled(false);
	//m_volMusicSlider->setEnabled(false);

	m_minCamAltitudeSpinner->setEnabled(false);
	m_maxCamAltitudeSpinner->setEnabled(false);

	m_minRenderDistSpinner->setEnabled(false);
	m_maxRenderDistSpinner->setEnabled(false);
}

void Options::buildOptionsPanel(CellStrip *container, int cell) {
	Lang &lang= Lang::getInstance();
	Config &config= Config::getInstance();

	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));
	Anchors padAnchors(Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, 10), Anchor(AnchorType::RIGID, 0));

	CellStrip *pnl = new CellStrip(container, Orientation::HORIZONTAL, 2);
	pnl->setCell(cell);
	pnl->setAnchors(fillAnchors);
	//pnl->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(1.f, 0.f, 1.f)));
	//pnl->borderStyle().setSizes(2);
	//TabWidget::add("Main", pnl);

	const int rows = 10;

	CellStrip *col1 = new CellStrip(pnl, Orientation::VERTICAL, Origin::FROM_TOP, rows);
	col1->setCell(0);
	col1->setAnchors(padAnchors);

	CellStrip *col2 = new CellStrip(pnl, Orientation::VERTICAL, Origin::FROM_TOP, rows);
	col2->setCell(1);
	col2->setAnchors(padAnchors);

	SizeHint hint(-1, int(g_widgetConfig.getDefaultItemHeight() * 1.5f));
	for (int i=0; i < rows; ++i) {
		col1->setSizeHint(i, hint);
		col2->setSizeHint(i, hint);
	}

	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::SPRINGY, 15));

	OptionWidget *dw = new OptionWidget(col1, lang.get("FxVolume"));
	dw->setCell(0);
	m_volFxSlider = new Slider2(dw, false);
	m_volFxSlider->setCell(1);
	m_volFxSlider->setAnchors(squashAnchors);
	m_volFxSlider->setRange(100);
	m_volFxSlider->setValue(clamp(config.getSoundVolumeFx(), 0, 100));
	m_volFxSlider->ValueChanged.connect(this, &Options::onSliderValueChanged);

	dw = new OptionWidget(col1, lang.get("AmbientVolume"));
	dw->setCell(1);
	m_volAmbientSlider = new Slider2(dw, false);
	m_volAmbientSlider->setCell(1);
	m_volAmbientSlider->setAnchors(squashAnchors);
	m_volAmbientSlider->setRange(100);
	m_volAmbientSlider->setValue(clamp(config.getSoundVolumeAmbient(), 0, 100));
	m_volAmbientSlider->ValueChanged.connect(this, &Options::onSliderValueChanged);

	dw = new OptionWidget(col1, lang.get("MusicVolume"));
	dw->setCell(2);
	m_volMusicSlider = new Slider2(dw, false);
	m_volMusicSlider->setCell(1);
	m_volMusicSlider->setAnchors(squashAnchors);
	m_volMusicSlider->setRange(100);
	m_volMusicSlider->setValue(clamp(config.getSoundVolumeMusic(), 0, 100));
	m_volMusicSlider->ValueChanged.connect(this, &Options::onSliderValueChanged);

	// Player Name
	dw = new OptionWidget(col1, lang.get("PlayerName"));
	dw->setCell(3);
	TextBox *tb = new TextBox(dw);
	tb->setText(g_config.getNetPlayerName());
	tb->setCell(1);
	tb->setAnchors(squashAnchors);
	tb->TextChanged.connect(this, &Options::onPlayerNameChanged);

	// Language
	dw = new OptionWidget(col1, lang.get("Language"));
	dw->setCell(4);
	m_langList = new DropList(dw);
	m_langList->setCell(1);
	m_langList->setAnchors(squashAnchors);
	setupListBoxLang();
	m_langList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	m_langList->setDropBoxHeight(200);

	// Debug mode/keys container
	DoubleOption *qw = new DoubleOption(col1, lang.get("DebugMode"), lang.get("DebugKeys"));
	qw->setCell(5);

	// Debug mode
	CheckBoxHolder *cbh = new CheckBoxHolder(qw);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);
	m_debugModeCheckBox = new CheckBox(cbh);
	m_debugModeCheckBox->setCell(1);
	m_debugModeCheckBox->setAnchors(squashAnchors);
	m_debugModeCheckBox->setChecked(config.getMiscDebugMode());
	m_debugModeCheckBox->Clicked.connect(this, &Options::onToggleDebugMode);

	// Debug keys
	cbh = new CheckBoxHolder(qw);
	cbh->setCell(3);
	cbh->setAnchors(fillAnchors);
	m_debugKeysCheckBox = new CheckBox(cbh);
	m_debugKeysCheckBox->setCell(1);
	m_debugKeysCheckBox->setAnchors(squashAnchors);
	m_debugKeysCheckBox->setChecked(config.getMiscDebugKeys());
	m_debugKeysCheckBox->Clicked.connect(this, &Options::onToggleDebugKeys);

	// Camera min / max altitude
	RelatedDoubleOption *rdo = new RelatedDoubleOption(col1,
		lang.get("CameraAltitude"), lang.get("Min"), lang.get("Max"));
	rdo->setCell(6);
	rdo->setCustomSplits(13, 14, 20);
	m_minCamAltitudeSpinner = new Spinner(rdo);
	m_minCamAltitudeSpinner->setCell(2);
	m_minCamAltitudeSpinner->setAnchors(squashAnchors);
	m_minCamAltitudeSpinner->setRanges(0, 20);
	m_minCamAltitudeSpinner->setIncrement(1);
	m_minCamAltitudeSpinner->setValue(int(config.getCameraMinDistance()));
	m_minCamAltitudeSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_maxCamAltitudeSpinner = new Spinner(rdo);
	m_maxCamAltitudeSpinner->setCell(4);
	m_maxCamAltitudeSpinner->setAnchors(squashAnchors);
	m_maxCamAltitudeSpinner->setRanges(32, 2048);
	m_maxCamAltitudeSpinner->setIncrement(32);
	m_maxCamAltitudeSpinner->setValue(int(config.getCameraMaxDistance()));
	m_maxCamAltitudeSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Shadows
	dw = new OptionWidget(col2, lang.get("Shadows"));
	dw->setCell(0);
	m_shadowsList = new DropList(dw);
	m_shadowsList->setCell(1);
	m_shadowsList->setAnchors(squashAnchors);
	for(int i= 0; i < ShadowMode::COUNT; ++i){
		m_shadowsList->addItem(lang.get(Renderer::shadowsToStr(ShadowMode(i))));
	}
	string str= config.getRenderShadows();
	m_shadowsList->setSelected(clamp(int(Renderer::strToShadows(str)), 0, ShadowMode::COUNT - 1));
	m_shadowsList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	// Texture filter
	dw = new OptionWidget(col2, lang.get("TextureFilter"));
	dw->setCell(1);
	m_filterList = new DropList(dw);
	m_filterList->setCell(1);
	m_filterList->setAnchors(squashAnchors);
	m_filterList->addItem("Bilinear");
	m_filterList->addItem("Trilinear");
	m_filterList->setSelected(config.getRenderFilter());
	m_filterList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	// lights
	qw = new DoubleOption(col2, lang.get("MaxLights"), lang.get("Textures3D"));
	qw->setCustomSplit(true, 35);
	qw->setCell(2);
	m_lightsList = new DropList(qw);
	m_lightsList->setCell(1);
	m_lightsList->setAnchors(squashAnchors);
	for (int i = 1; i <= 8; ++i) {
		m_lightsList->addItem(intToStr(i));
	}
	m_lightsList->setSelected(clamp(config.getRenderLightsMax()-1, 0, 7));
	m_lightsList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
	m_lightsList->setDropBoxHeight(200);

	// 3D Textures
	cbh = new CheckBoxHolder(qw);
	cbh->setCell(3);
	m_3dTexCheckBox = new CheckBox(cbh);
	m_3dTexCheckBox->setCell(1);
	m_3dTexCheckBox->setAnchors(squashAnchors);
	m_3dTexCheckBox->setChecked(config.getRenderTextures3D());
	m_3dTexCheckBox->Clicked.connect(this, &Options::on3dTexturesToggle);

	// Camera min / max altitude
	rdo = new RelatedDoubleOption(col2,
		lang.get("RenderDistance"), lang.get("Min"), lang.get("Max"));
	rdo->setCell(3);
	rdo->setCustomSplits(13, 14, 20);
	m_minRenderDistSpinner = new Spinner(rdo);
	m_minRenderDistSpinner->setCell(2);
	m_minRenderDistSpinner->setAnchors(squashAnchors);
	m_minRenderDistSpinner->setRanges(0, 20);
	m_minRenderDistSpinner->setIncrement(1);
	m_minRenderDistSpinner->setValue(int(config.getRenderDistanceMin()));
	m_minRenderDistSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	m_maxRenderDistSpinner = new Spinner(rdo);
	m_maxRenderDistSpinner->setCell(4);
	m_maxRenderDistSpinner->setAnchors(squashAnchors);
	m_maxRenderDistSpinner->setRanges(32, 4096);
	m_maxRenderDistSpinner->setIncrement(32);
	m_maxRenderDistSpinner->setValue(int(config.getRenderDistanceMax()));
	m_maxRenderDistSpinner->ValueChanged.connect(this, &Options::onSpinnerValueChanged);

	// Field of View
	dw = new OptionWidget(col2, lang.get("RenderFoV"));
	dw->setCell(4);

	// Enable Shaders
	dw = new OptionWidget(col2, lang.get("UseShaders"));
	dw->setCell(5);
	cbh = new CheckBoxHolder(dw);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);
	CheckBox *checkBox  = new CheckBox(cbh);
	checkBox->setCell(1);
	checkBox->setAnchors(squashAnchors);
	checkBox->setChecked(config.getRenderUseShaders());
	checkBox->Clicked.connect(this, &Options::onToggleShaders);
	if (g_config.getRenderTestingShaders()) {
		checkBox->setEnabled(false);
	}

	// Terrain Shader
	dw = new OptionWidget(col2, lang.get("TerrainShader"));
	dw->setCell(6);
	
	// Water Shader
	dw = new OptionWidget(col2, lang.get("WaterShader"));
	dw->setCell(7);

	// Model Shader
	dw = new OptionWidget(col2, lang.get("ModelShader"));
	dw->setCell(8);

	m_modelShaderList = new DropList(dw);
	m_modelShaderList->setCell(1);
	m_modelShaderList->setAnchors(squashAnchors);
	m_modelShaderList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);

	loadShaderList();
}

void Options::loadShaderList() {
	m_modelShaderList->clearItems();
	if (g_config.getRenderUseShaders() && !g_config.getRenderTestingShaders()) {
		findAll("/gae/shaders/*.xml", m_modelShaders, true, false);
		string currentName = g_config.getRenderModelShader();
		int currentNdx = -1, i = 0;
		foreach (vector<string>, it, m_modelShaders) {
			if (currentName == *it) {
				currentNdx = i;
			}
			m_modelShaderList->addItem(formatString(*it));
			++i;
		}
		m_modelShaderList->SelectionChanged.disconnect(this);
		m_modelShaderList->setSelected(currentNdx);
		m_modelShaderList->SelectionChanged.connect(this, &Options::onDropListSelectionChanged);
		m_modelShaderList->setEnabled(true);
	} else {
		m_modelShaderList->setEnabled(false);
	}
}

void Options::onToggleShaders(Widget*) {
	g_config.setRenderUseShaders(!g_config.getRenderUseShaders());
	loadShaderList();
	if (!g_config.getRenderUseShaders()) {
		g_renderer.changeShader("");
	}
}

void Options::onToggleDebugMode(Widget*) {
	g_config.setMiscDebugMode(m_debugModeCheckBox->isChecked());
	if (m_optionsMenu) {
		m_optionsMenu->showDebugText(m_debugModeCheckBox->isChecked());
	}
	save();
}

void Options::onToggleDebugKeys(Widget*) {
	g_config.setMiscDebugKeys(m_debugKeysCheckBox->isChecked());
}

void Options::on3dTexturesToggle(Widget*) {
	Config &config= Config::getInstance();
	config.setRenderTextures3D(m_3dTexCheckBox->isChecked());
// 	saveConfig();
}

void Options::onSpinnerValueChanged(Widget *source) {
	Spinner *spinner = static_cast<Spinner*>(source);
	if (spinner == m_minCamAltitudeSpinner) {
		g_config.setCameraMinDistance(float(spinner->getValue()));
	} else if (spinner == m_maxCamAltitudeSpinner) {
		g_config.setCameraMaxDistance(float(spinner->getValue()));
	} else if (spinner == m_minRenderDistSpinner) {
		g_config.setRenderDistanceMin(float(spinner->getValue()));
	} else if (spinner == m_maxRenderDistSpinner) {
		g_config.setRenderDistanceMax(float(spinner->getValue()));
	}
}

void Options::onPlayerNameChanged(Widget *source) {
	TextBox *tb = static_cast<TextBox*>(source);
	g_config.setNetPlayerName(tb->getText());
}

void Options::onDropListSelectionChanged(Widget *source) {
	ListBase *list = static_cast<ListBase*>(source);
	Config &config= Config::getInstance();

	if (list == m_langList) {
		map<string,string>::iterator it = m_langMap.find(list->getSelectedItem()->getText());
		string lng;
		if ( it != m_langMap.end() ) {
			lng = it->second;
		} else {
			lng = list->getSelectedItem()->getText();
		}
		config.setUiLocale(lng);
		if (m_optionsMenu) {
			m_optionsMenu->reload();
		} else {
			///@todo reload user_interface? - hailstone 14May2011
		}
	} else if (list == m_filterList) {
		config.setRenderFilter(m_filterList->getSelectedItem()->getText());
// 		saveConfig();
	} else if (list == m_shadowsList) {
		int index = m_shadowsList->getSelectedIndex();
		config.setRenderShadows(Renderer::shadowsToStr(ShadowMode(index)));
// 		saveConfig();
	} else if (list == m_lightsList) {
		config.setRenderLightsMax(list->getSelectedIndex() + 1);
// 		saveConfig();
	} else if (list = m_modelShaderList) {
		string shader = m_modelShaders[list->getSelectedIndex()];
		g_renderer.changeShader(shader);
		g_config.setRenderModelShader(shader);
	}
}

void Options::onSliderValueChanged(Widget *source) {
	Slider2 *slider = static_cast<Slider2*>(source);
	if (slider == m_volFxSlider) {
		g_config.setSoundVolumeFx(slider->getValue());
		g_soundRenderer.setFxVolume(slider->getValue() / 100.f);
	} else if (slider == m_volAmbientSlider) {
		g_config.setSoundVolumeAmbient(slider->getValue());
		g_soundRenderer.setAmbientVolume(slider->getValue() / 100.f);
	} else if (slider == m_volMusicSlider) {
		g_config.setSoundVolumeMusic(slider->getValue());
		g_soundRenderer.setMusicVolume(slider->getValue() / 100.f);
	}
}

// private

void Options::save() {
	Config &config= Config::getInstance();

	config.save();
	Renderer::getInstance().loadConfig();
	SoundRenderer::getInstance().loadConfig();
}

void Options::setupListBoxLang() {
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
// 	class OptionsFrame
// =====================================================

OptionsFrame::OptionsFrame(WidgetWindow* window)
		: Frame(window, ButtonFlags::CLOSE)
		, m_saveButton(0)
		, m_options(0) {
	init();
}

OptionsFrame::OptionsFrame(Container* parent)
		: Frame(parent, ButtonFlags::CLOSE)
		, m_saveButton(0)
		, m_options(0) {
	init();
}

void OptionsFrame::init() {

	// apply/cancel buttons

	// options panel
	setSizeHint(1, SizeHint());
	m_optionsPanel = new CellStrip(this, Orientation::HORIZONTAL, 1);
	m_optionsPanel->setCell(1);
	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_optionsPanel->setAnchors(anchors);

	m_options = new Options(m_optionsPanel, 0);
	m_options->setCell(0);
	m_options->setAnchors(anchors);

	setDirty();
}

void OptionsFrame::init(Vec2i pos, Vec2i size, const string &title) {
	setPos(pos);
	setSize(size);
	setTitleText(title);
	layoutCells();
}

void OptionsFrame::onButtonClicked(Widget *source) {
	/*Button *btn = static_cast<Button*>(source);
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}*/
}

}}//end namespace