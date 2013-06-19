// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "misc_widgets.h"
#include "widget_window.h"
#include "character_creator.h"

namespace Glest { namespace Widgets {


// =====================================================
//  class OptionContainer
// =====================================================

OptionContainer::OptionContainer(Container* parent, Vec2i pos, Vec2i size, const string &text)
		: Container(parent, pos, size) {
	setWidgetStyle(WidgetType::TEXT_BOX);
	m_abosulteLabelSize = false;
	m_labelSize = 30;
	int w = int(size.x * float(30) / 100.f);
	m_label = new StaticText(this, Vec2i(0), Vec2i(w, size.y));
	m_label->setText(text);
	m_widget = 0;
}

void OptionContainer::setWidget(Widget* widget) {
	m_widget = widget;
	Vec2i size = getSize();
	int w;
	if (m_abosulteLabelSize) {
		w = m_labelSize;
	} else {
		w = int(size.x * float(m_labelSize) / 100.f);
	}
	m_label->setSize(Vec2i(w, size.y));
	if (m_widget) {
		m_widget->setPos(Vec2i(w,0));
		m_widget->setSize(Vec2i(size.x - w, size.y));
	}
}

void OptionContainer::setLabelWidth(int value, bool absolute) {
	m_abosulteLabelSize = absolute;
	m_labelSize = value;
	Vec2i size = getSize();
	int w;
	if (m_abosulteLabelSize) {
		w = m_labelSize;
	} else {
		w = int(size.x * float(m_labelSize) / 100.f);
	}
	m_label->setSize(Vec2i(w, size.y));
	if (m_widget) {
		m_widget->setPos(Vec2i(w,0));
		m_widget->setSize(Vec2i(size.x - w, size.y));
	}
}

Vec2i OptionContainer::getPrefSize() const {
	return Vec2i(700, 40);
}

Vec2i OptionContainer::getMinSize() const {
	return Vec2i(400, 30);
}

// =====================================================
//  class ScrollText
// =====================================================

ScrollText::ScrollText(Container* parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 1)
		, TextWidget(this) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::TEXT_BOX));
	init();
}

ScrollText::ScrollText(Container* parent, Vec2i pos, Vec2i size)
		: CellStrip(parent, pos, size, Orientation::HORIZONTAL, Origin::CENTRE, 1)
		, TextWidget(this) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::TEXT_BOX));
	init();
	recalc();
}

void ScrollText::init() {
	int itemSize = m_rootWindow->getConfig()->getDefaultItemHeight();

	// Anchors for scroll-bar, stick to the top, right & bottom sides.
	// Not anchored to left border, so we must set size (width will be respected, height will not)
	Anchors anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::RIGID, 0), // left, top
		Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0));           // right, bottom

	m_scrollBar = new ScrollBar(this, true, 10);
	m_scrollBar->setCell(0);
	m_scrollBar->setAnchors(anchors);
	m_scrollBar->setSize(Vec2i(itemSize));
	m_scrollBar->setVisible(false);
	m_scrollBar->ThumbMoved.connect(this, &ScrollText::onScroll);

	// Anchors for text widget...
	// with scroll-bar stick to left, top & bottom, and 'itemSize' in from right
	m_anchorWithScroll = Anchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, itemSize), Anchor(AnchorType::RIGID, 0));
	// with no scroll-bar, fill cell
	m_anchorNoScroll = Anchors(Anchor(AnchorType::RIGID, 0));

	m_staticText = new StaticText(this);
	m_staticText->setCell(0);
	m_staticText->setAnchors(m_anchorNoScroll);
	m_staticText->setAlignment(Alignment::NONE);
	m_staticText->textStyle().m_fontIndex = m_textStyle.m_fontIndex;
	m_staticText->setText("");
}

void ScrollText::recalc() {
	int th = m_staticText->getTextDimensions().h;
	int ch = m_staticText->getHeight() - m_staticText->getBordersVert();
	m_scrollBar->setRanges(th, ch);
}

void ScrollText::onScroll(ScrollBar*) {
	int ox = m_staticText->getPos().x;
	m_staticText->setPos(Vec2i(ox, -round(m_scrollBar->getThumbPos())));
}

void ScrollText::setAndWrapText(const string &txt) {
	m_origString = txt;
	string text = txt;
	int width = getSize().w - getBordersHoriz() - m_staticText->getBordersHoriz();
	const FontMetrics *fm = m_staticText->getFont()->getMetrics();
	fm->wrapText(text, width);
	// try to fit with no scroll-bar,
	if (fm->getTextDiminsions(text).h < getHeight() - getBordersVert() - m_staticText->getBordersVert()) {
		m_scrollBar->setVisible(false);
		m_staticText->setAnchors(m_anchorNoScroll);
	} else { // else re-wrap, taking into account scroll-bar width
		m_scrollBar->setVisible(true);
		m_staticText->setAnchors(m_anchorWithScroll);
		width = getSize().w - m_scrollBar->getSize().w - getBordersHoriz() - m_staticText->getBordersHoriz();
		text = txt;
		fm->wrapText(text, width);
	}
	layoutCells();
	m_staticText->setText(text);
}

void ScrollText::setSize(const Vec2i &sz) {
	const FontMetrics *fm = g_widgetConfig.getFont(m_staticText->textStyle().m_fontIndex)->getMetrics();
	CellStrip::setSize(sz);
	layoutCells(); // force layout
	if (!m_origString.empty()) {
		setAndWrapText(m_origString);
	}
	recalc();
}

void ScrollText::setText(const string &txt, ScrollAction scroll) {
	float oldOffset = m_scrollBar->getThumbPos();

	setAndWrapText(txt);
	recalc();

	if (scroll == ScrollAction::BOTTOM) {
		m_scrollBar->setThumbPosPercent(100);
	} else if (scroll == ScrollAction::MAINTAIN) {
		//WIDGET_LOG( "ScrollText::setText() : Restoring offset: " << oldOffset );
		m_scrollBar->setThumbPos(oldOffset);
	} else {
		m_scrollBar->setThumbPos(0);
	}
}

void ScrollText::render() {
	CellStrip::render();
}


// =====================================================
// class InputBox
// =====================================================

InputBox::InputBox(Container *parent)
		: TextBox(parent) {
}

//InputBox::InputBox(Container *parent, Vec2i pos, Vec2i size)
//		: TextBox(parent, pos, size){
//}

bool InputBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::ESCAPE:
			Escaped(this);
			return true;
	}
	return TextBox::keyDown(key);
}

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
// 	class TraitsDisplay
// =====================================================
TraitsDisplay::TraitsDisplay(Container *parent, Traits traitlist, ListSpecs specs, CharacterCreator *characterCreator)
		: Widget(parent)
		, MouseWidget(this)
		, TextWidget(this)
		, charCreator(characterCreator)
		, traits(traitlist)
		, specializations(specs)
		, m_fuzzySize(FuzzySize::SMALL) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);
	TextWidget::addText("");
    layout();
	clear();
	CHECK_HEAP();
}

void TraitsDisplay::layout() {
	int x = 0;
	int y = 0;

	const Font *font = getSmallFont();
	int fontIndex = m_textStyle.m_smallFontIndex != -1 ? m_textStyle.m_smallFontIndex : m_textStyle.m_fontIndex;

	if (m_fuzzySize== FuzzySize::SMALL) {
		font = getSmallFont();
		fontIndex = m_textStyle.m_smallFontIndex != -1 ? m_textStyle.m_smallFontIndex : m_textStyle.m_fontIndex;
	} else if (m_fuzzySize == FuzzySize::MEDIUM) {
		font = getFont();
		fontIndex = m_textStyle.m_fontIndex;
	} else if (m_fuzzySize == FuzzySize::LARGE) {
		font = getBigFont();
		fontIndex = m_textStyle.m_largeFontIndex != -1 ? m_textStyle.m_largeFontIndex : m_textStyle.m_fontIndex;
	}
	m_fontMetrics = font->getMetrics();


    string str = "";
    if (charCreator->getSovereignState() == false) {
        int j = 0;
        for (int i = 0; i < specializations.size(); ++i) {
            if (specializations[i]->getSpecName() == charCreator->getCurrentSpec()) {
                j = i;
                break;
            }
        }
        str += specializations[j]->getSpecName();
        str += "\n";
        str += intToStr(specializations[j]->getCreatorCost()->getValue());
        str += "\n";
        specializations[j]->getCharacterStats()->getDesc(str, "\n");
        specializations[j]->getStatistics()->enhancement.getDesc(str, "\n");
        for (int i = 0; i < specializations[j]->getEquipmentCount(); ++i) {
            Equipment *equip = specializations[j]->getEquipment(i);
            equip->getDesc(str, "\n", "");
        }
        specializations[j]->getKnowledge()->getDesc(str, "\n");
    } else if (charCreator->getSovereignState() == true) {
        int j = 0;
        for (int i = 0; i < traits.size(); ++i) {
            if (traits[i]->getName() == charCreator->getCurrentTrait()) {
                j = i;
                break;
            }
        }
        traits[j]->getDesc(str, "\n");
    }

    TextWidget::setTextPos(Vec2i(5, y), 0);
    TextWidget::setText(str, 0);

	for (int i=0; i < 1; ++i) {
		setTextFont(fontIndex, i);
	}
}

void TraitsDisplay::persist() {
	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;
}

void TraitsDisplay::reset() {
    layout();
}

void TraitsDisplay::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void TraitsDisplay::setSize() {
}

void TraitsDisplay::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );
}

void TraitsDisplay::render() {
	if (!isVisible()) {
		return;
	}
	Widget::render();
	if (!TextWidget::getText(0).empty()) {
		TextWidget::renderTextShadowed(0);
	}
}

// =====================================================
// 	class SkillsDisplay
// =====================================================
SkillsDisplay::SkillsDisplay(Container *parent, Actions *newActions, CharacterCreator *characterCreator)
		: Widget(parent)
		, MouseWidget(this)
		, TextWidget(this)
		, ImageWidget(this)
		, m_imageSize(32)
		, m_hoverBtn(SkillSection::INVALID, invalidIndex)
		, m_pressedBtn(SkillSection::INVALID, invalidIndex)
		, charCreator(characterCreator)
		, actions(newActions)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);
	for (int i = 0; i < skillCellCount; ++i) {
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}
    for (int i = 0; i < actions->getCommandTypeCount(); ++i) {
        if (actions->getCommandType(i)->getClass() == CmdClass::ATTACK) {
            MoveBaseCommandType *mbct = static_cast<MoveBaseCommandType*>(actions->getCommandType(i));
            mbct->initMoveSkill(actions);
        }
        if (actions->getCommandType(i)->getClass() == CmdClass::ATTACK) {
            AttackCommandType *act = static_cast<AttackCommandType*>(actions->getCommandType(i));
            act->initAttackSkill(actions);
            act->attackSkillsInit();
        }
    }
    layout();
	clear();
	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);
    m_toolTip->setPos(m_toolTip->getPos() + Vec2i(220, 85));
	CHECK_HEAP();
}

void SkillsDisplay::layout() {
	int x = 0;
	int y = 0;

	const Font *font = getSmallFont();
	int fontIndex = m_textStyle.m_smallFontIndex != -1 ? m_textStyle.m_smallFontIndex : m_textStyle.m_fontIndex;

	if (m_fuzzySize== FuzzySize::SMALL) {
		font = getSmallFont();
		fontIndex = m_textStyle.m_smallFontIndex != -1 ? m_textStyle.m_smallFontIndex : m_textStyle.m_fontIndex;
	} else if (m_fuzzySize == FuzzySize::MEDIUM) {
		font = getFont();
		fontIndex = m_textStyle.m_fontIndex;
	} else if (m_fuzzySize == FuzzySize::LARGE) {
		font = getBigFont();
		fontIndex = m_textStyle.m_largeFontIndex != -1 ? m_textStyle.m_largeFontIndex : m_textStyle.m_fontIndex;
	}
	m_fontMetrics = font->getMetrics();

	for (int i = 0; i < skillCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	for (int i=0; i < 0; ++i) {
		setTextFont(fontIndex, i);
	}
}

void SkillsDisplay::computeSkillPanel() {
    for (int i = 0; i < actions->getCommandTypeCount(); ++i) {
        CommandType *ct = actions->getCommandType(i);
        setSkillImage(i, ct->getImage());
        setCommandType(i, ct);
    }
}

void SkillsDisplay::computeSkillInfo(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
    const CommandType *ct = commandTypes[posDisplay];
    if (ct != 0) {
        m_toolTip->clearItems();
        string header = ct->getName();
        m_toolTip->setHeader(header);
        m_toolTip->setTipText("");
        const SkillCosts *costs = ct->getSkillCosts();
        costs->getDesc(m_toolTip);
        const AttackCommandType *act = 0;
        if (ct->getClass() == CmdClass::ATTACK) {
            act = static_cast<const AttackCommandType*>(ct);
            string str = "";
            //act->getAttackSkillTypes()->getDesc(str, 0);
            act->descSkills(0, m_toolTip);
            m_toolTip->addElement(str);
        }
        //ct->descSkills(0, m_toolTip);
    }
    m_toolTip->setVisible(true);
}

void SkillsDisplay::persist() {
	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;
}

void SkillsDisplay::reset() {
    layout();
}

void SkillsDisplay::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void SkillsDisplay::setSize() {
}

void SkillsDisplay::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < skillCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}
}

void SkillsDisplay::setToolTipText(const string &hdr, const string &tip, SkillSection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
}

void SkillsDisplay::render() {
	if (!isVisible()) {
		return;
	}
	Widget::render();	Widget::render();
	ImageWidget::startBatch();

	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);
	for (int i = 0; i < skillCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
		}
	}
	ImageWidget::endBatch();
}

SkillButton SkillsDisplay::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	int counts[1] = {skillCellCount};

	for (int i = 0; i < 1; ++i) {
		pos = i_pos;
		if (pos.y >= 0 && pos.y < m_imageSize * (counts[i]/6)) {
			int cellX = pos.x / m_imageSize;
			int cellY = (pos.y / m_imageSize) % (counts[i]/6);
			int index = cellY * cellWidthCount + cellX;
			if (index >= 0 && index < counts[i]) {
                int totalCells = 0;
			    for (int l = i - 1; l >= 0; --l) {
                    totalCells += counts[l];
			    }
				if (ImageWidget::getImage(totalCells + index)) {
					return SkillButton(SkillSection(i), index);
				}
				return SkillButton(SkillSection::INVALID, invalidIndex);
			}
		}
	}
	return SkillButton(SkillSection::INVALID, invalidIndex);
}

void SkillsDisplay::skillButtonPressed(int posDisplay) {
	WIDGET_LOG( __FUNCTION__ << "( " << posDisplay << " )");
    string actionName = commandTypes[posDisplay]->getName();
    charCreator->addActions(actions, actionName);
    computeSkillInfo(posDisplay);
}

bool SkillsDisplay::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == SkillSection::SKILLS) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else {
				m_pressedBtn = SkillButton(SkillSection::INVALID, invalidIndex);
			}
		}
	}
	return false;
}

bool SkillsDisplay::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != SkillSection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == SkillSection::SKILLS) {
						skillButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = SkillButton(SkillSection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = SkillButton(SkillSection::INVALID, invalidIndex);
		}
	}
	return false;
}

bool SkillsDisplay::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		SkillButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == SkillSection::SKILLS) {
				computeSkillInfo(currBtn.m_index);
			} else {
				setToolTipText("", "", SkillSection::SKILLS);
			}
			m_hoverBtn = currBtn;
			return true;
		} else {
		}
	} else {
		setToolTipText("", "", SkillSection::SKILLS);
	}
	return false;
}

bool SkillsDisplay::mouseDoubleClick(MouseButton btn, Vec2i pos) {
    return false;
}

void SkillsDisplay::mouseOut() {

}

// =====================================================
//  class OptionPanel
// =====================================================
OptionPanel::OptionPanel(CellStrip *parent, int cell)
		: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2), m_scrollOffset(0)
		, MouseWidget(this) {
	setCell(cell);
	setWidgetStyle(WidgetType::OPTIONS_PANEL);
	Anchors anchors(Anchor(AnchorType::SPRINGY, 1));
	setAnchors(anchors);

	int sbw = g_widgetConfig.getDefaultItemHeight();

	m_noScrollSizeHint = SizeHint(0);
	m_scrollSizeHint = SizeHint(-1, sbw);

	m_list = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP, 0);
	m_list->setCell(0);
	m_list->setAnchors(Anchors::getFillAnchors());
	setSizeHint(0, SizeHint());

	m_scrollBar = new ScrollBar(this, true, sbw);
	m_scrollBar->setCell(1);
	m_scrollBar->setAnchors(Anchors::getFillAnchors());
	m_scrollBar->ThumbMoved.connect(this, &OptionPanel::onScroll);
	setSizeHint(1, m_noScrollSizeHint);
	m_splitDistance = 50;
}

void OptionPanel::setSize(const Vec2i &sz) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int req_h = h * m_list->getCellCount();
	if (req_h > sz.h - getBordersVert()) {
		setSizeHint(1, m_scrollSizeHint);
		m_scrollBar->setVisible(true);
		m_scrollBar->setRanges(req_h, sz.h - getBordersVert());
		m_scrollBar->setThumbPos(float(m_scrollOffset));
	} else {
		setSizeHint(1, m_noScrollSizeHint);
		m_scrollBar->setVisible(false);
	}
	CellStrip::setSize(sz);
	CellStrip::layoutCells();
	m_list->layoutCells();
	m_origPositions.resize(m_list->getChildCount());
	for (int i=0; i < m_list->getChildCount(); ++i) {
		m_origPositions[i] = m_list->getChild(i)->getPos();
		m_list->getChild(i)->setPos(Vec2i(m_origPositions[i].x, m_origPositions[i].y - m_scrollOffset));
	}
}

void OptionPanel::onScroll(ScrollBar *sb) {
	m_scrollOffset = int(sb->getThumbPos());
	for (int i=0; i < m_list->getChildCount(); ++i) {
		m_list->getChild(i)->setPos(Vec2i(m_origPositions[i].x, m_origPositions[i].y - m_scrollOffset));
	}
}

void OptionPanel::onHeadingClicked(Widget *widget) {
	ListBoxItem *heading = static_cast<ListBoxItem*>(widget);
	// need to set to start so the heading position is accurate
	setScrollPosition(0);
	setScrollPosition(float(m_headings[heading->getText()]->getPos().y));
}

ListBoxItem* OptionPanel::addHeading(OptionPanel* headingPnl, const string &txt) {
	ListBoxItem *link = headingPnl->addLabel(" " + txt);
	link->Clicked.connect(this, &OptionPanel::onHeadingClicked);

	ListBoxItem *heading = addLabel(" " + txt);
	m_headings.insert(std::pair<string, ListBoxItem*>(" " + txt, heading));

	return heading;
}

ListBoxItem* OptionPanel::addLabel(const string &txt) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	Anchors fillAnchors(Anchor(AnchorType::RIGID, 0));

	ListBoxItem *label = new ListBoxItem(m_list); //hacky change from StaticText to use WidgetType::LIST_ITEM style - hailstone
	label->setAlignment(Alignment::FLUSH_LEFT);
	label->setAnchors(fillAnchors);
	label->setCell(m_list->getCellCount());
	label->setText(txt);
	m_list->addCells(1);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));
	return label;
}

CheckBox* OptionPanel::addCheckBox(const string &txt, bool checked) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	OptionWidget *ow = new OptionWidget(m_list, txt);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setPercentSplit(m_splitDistance);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));

	CheckBoxHolder *cbh = new CheckBoxHolder(ow);
	cbh->setCell(1);
	cbh->setAnchors(fillAnchors);

	CheckBox *checkBox  = new CheckBox(cbh);
	checkBox->setCell(1);
	checkBox->setAnchors(squashAnchors);
	checkBox->setChecked(checked);

	return checkBox;
}

TextBox* OptionPanel::addTextBox(const string &lbl, const string &txt) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	OptionWidget *ow = new OptionWidget(m_list, lbl);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setPercentSplit(m_splitDistance);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));

	TextBox *textBox = new TextBox(ow);
	textBox->setCell(1);
	textBox->setAnchors(squashAnchors);
	textBox->setText(txt);

	return textBox;
}

TraitsDisplay* OptionPanel::addTraitsDisplay(Traits traits, ListSpecs specs, CharacterCreator *characterCreator) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	CellStrip *ow = new CellStrip(m_list, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setSizeHint(0, 0);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h*11));

	TraitsDisplay *traitsDisplay = new TraitsDisplay(ow, traits, specs, characterCreator);
	traitsDisplay->setCell(1);
	traitsDisplay->setAnchors(squashAnchors);

	return traitsDisplay;
}

SkillsDisplay* OptionPanel::addSkillsDisplay(Actions *actions, CharacterCreator *characterCreator) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	CellStrip *ow = new CellStrip(m_list, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setSizeHint(0, 0);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h*11));

	SkillsDisplay *skillsDisplay = new SkillsDisplay(ow, actions, characterCreator);
	skillsDisplay->setCell(1);
	skillsDisplay->setAnchors(squashAnchors);

	return skillsDisplay;
}

DropList* OptionPanel::addDropList(const string &lbl, bool compact) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	OptionWidget *ow = new OptionWidget(m_list, lbl);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setPercentSplit(m_splitDistance);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));

	if (compact) {
		squashAnchors = Anchors(Anchor(AnchorType::SPRINGY, 25), Anchor(AnchorType::RIGID, squashAmount));
	}
	DropList *dropList = new DropList(ow);
	dropList->setCell(1);
	dropList->setAnchors(squashAnchors);

	return dropList;
}

Spinner* OptionPanel::addSpinner(const string &lbl) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::SPRINGY, 25), Anchor(AnchorType::RIGID, squashAmount));

	OptionWidget *ow = new OptionWidget(m_list, lbl);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setPercentSplit(m_splitDistance);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));

	Spinner *spinner = new Spinner(ow);
	spinner->setCell(1);
	spinner->setAnchors(squashAnchors);

	return spinner;
}

SpinnerPair OptionPanel::addSpinnerPair(const string &lbl, const string &lbl1, const string &lbl2) {
	int h  = int(g_widgetConfig.getDefaultItemHeight() * 1.5f);
	int squashAmount = (h - g_widgetConfig.getDefaultItemHeight()) / 2;
	Anchors fillAnchors(Anchor(AnchorType::SPRINGY, 2));
	Anchors squashAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, squashAmount));

	OptionWidget *ow = new OptionWidget(m_list, lbl);
	m_list->addCells(1);
	ow->setAnchors(fillAnchors);
	ow->setCell(m_list->getCellCount() - 1);
	ow->setPercentSplit(m_splitDistance);
	m_list->setSizeHint(m_list->getCellCount() - 1, SizeHint(-1, h));

	DoubleOption *dow = new DoubleOption(ow, lbl1, lbl2);
	ow->setOptionWidget(dow);

	Spinner *spinner1 = new Spinner(dow);
	dow->setOptionWidget(true, spinner1);
	spinner1->setAnchors(squashAnchors);

	Spinner *spinner2 = new Spinner(dow);
	dow->setOptionWidget(false, spinner2);
	spinner2->setAnchors(squashAnchors);

	dow->setCustomSplit(true, 20);
	dow->setCustomSplit(false, 20);
	return std::make_pair(spinner1, spinner2);
}


}}

