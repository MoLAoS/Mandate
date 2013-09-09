// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "event_display.h"

#include "metrics.h"
#include "command_type.h"
#include "widget_window.h"
#include "core_data.h"
#include "user_interface.h"
#include "world.h"
#include "sim_interface.h"
#include "config.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

using Global::CoreData;

// =====================================================
//  class EventDisplayFrame
// =====================================================

EventDisplayFrame::EventDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::CLOSE | ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new EventWindow(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &EventDisplayFrame::onExpand);
	Shrink.connect(this, &EventDisplayFrame::onShrink);
	Close.connect(this, &EventDisplayFrame::remove);
	setPinned(g_config.getUiPinWidgets());
}

void EventDisplayFrame::remove(Widget*) {
    setVisible(false);
}

void EventDisplayFrame::resetSize() {
	if (m_display->isVisible()) {
		if (!isVisible()) {
			setVisible(true);
		}
		Vec2i size = m_display->getSize() + getBordersAll() + Vec2i(0, 20);
		if (size != getSize()){
			setSize(size);
		}
	} else {
		remove(this);
	}
}

void EventDisplayFrame::onExpand(Widget*) {
	assert(m_display->getFuzzySize() != FuzzySize::LARGE);
	FuzzySize sz = m_display->getFuzzySize();
	++sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_display->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void EventDisplayFrame::onShrink(Widget*) {
	assert(m_display->getFuzzySize() != FuzzySize::SMALL);
	FuzzySize sz = m_display->getFuzzySize();
	--sz;
	assert(sz > FuzzySize::INVALID && sz < FuzzySize::COUNT);
	m_display->setFuzzySize(sz);
	if (sz == FuzzySize::SMALL) {
		enableShrinkExpand(false, true);
	} else if (sz == FuzzySize::MEDIUM) {
		enableShrinkExpand(true, true);
	} else if (sz == FuzzySize::LARGE) {
		enableShrinkExpand(true, false);
	}
}

void EventDisplayFrame::render() {
	if (g_config.getUiPhotoMode()) {
		return;
	}
	if (!m_display->isVisible()) {
        setVisible(false);
	} else if (m_display->isVisible()) {
        setVisible(true);
	}
	Frame::render();
}

void EventDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class EventWindow
// =====================================================

EventWindow::EventWindow(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(880, 5))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_imageSize(32)
		, m_hoverBtn(EventDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(EventDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);

	for (int i = 0; i < pictureCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < titleCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < choicesCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

    TextWidget::setAlignment(Alignment::NONE);
	TextWidget::setText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");
	TextWidget::addText("");

	const Texture2D* overlayImages[3] = {
		g_widgetConfig.getTickTexture(),
		g_widgetConfig.getCrossTexture(),
		g_widgetConfig.getQuestionTexture()
	};
	layout();
	clear();
	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);
	CHECK_HEAP();
}

void EventWindow::layout() {
	int x = 0;
	int y = 0;

	const Font *font = getTinyFont();
	int fontIndex = m_textStyle.m_tinyFontIndex != -1 ? m_textStyle.m_tinyFontIndex : m_textStyle.m_smallFontIndex;

	if (m_fuzzySize== FuzzySize::SMALL) {
		m_imageSize = 32;
		font = getTinyFont();
		fontIndex = m_textStyle.m_tinyFontIndex != -1 ? m_textStyle.m_tinyFontIndex : m_textStyle.m_smallFontIndex;
	} else if (m_fuzzySize == FuzzySize::MEDIUM) {
		m_imageSize = 48;
		font = getSmallFont();
		fontIndex = m_textStyle.m_smallFontIndex;
	} else if (m_fuzzySize == FuzzySize::LARGE) {
		m_imageSize = 64;
		font = getFont();
		fontIndex = m_textStyle.m_fontIndex != -1 ? m_textStyle.m_fontIndex : m_textStyle.m_fontIndex;
	}
	m_fontMetrics = font->getMetrics();

	x = 0;
	y = 0;
	m_sizes.displaySize = Vec2i(m_imageSize * 6, m_imageSize*14);
	for (int i = 0; i < pictureCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	x = 0;
	y += m_imageSize;
    TextWidget::setTextPos(Vec2i(5, y), 0);
	for (int i = 0; i < titleCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, pictureCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

    x = 0;
    y += m_imageSize;
	m_choicesOffset = Vec2i(x, y);
	for (int i = 0; i < 8; ++i) {
        TextWidget::setTextPos(Vec2i(50, y+i*m_imageSize), i+1);
	}
	for (int i = 0; i < choicesCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, pictureCellCount + titleCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	for (int i=0; i < 9; ++i) {
		setTextFont(fontIndex, i);
	}
}

void EventWindow::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastEventDisplaySize(sz);
	cfg.setUiLastEventDisplayPosX(pos.x);
	cfg.setUiLastEventDisplayPosY(pos.y);
}

void EventWindow::reset() {
	Config &cfg = g_config;
	cfg.setUiLastEventDisplaySize(2);
	cfg.setUiLastEventDisplayPosX(400);
	cfg.setUiLastEventDisplayPosY(50);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<EventDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<EventDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void EventWindow::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void EventWindow::setSize() {
	Vec2i sz = m_sizes.displaySize;
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<EventDisplayFrame*>(m_parent)->resetSize();
}

void EventWindow::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < pictureCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}

	for (int i=0; i < titleCellCount; ++i) {
		ImageWidget::setImage(0, i + pictureCellCount);
	}

	for (int i=0; i < choicesCellCount; ++i) {
		ImageWidget::setImage(0, i + pictureCellCount + titleCellCount);
	}

	TextWidget::setText("", 0);
	TextWidget::setText("", 1);
	TextWidget::setText("", 2);
	TextWidget::setText("", 3);
	TextWidget::setText("", 4);
	TextWidget::setText("", 5);
	TextWidget::setText("", 6);
	TextWidget::setText("", 7);
	TextWidget::setText("", 8);
}

void EventWindow::render() {
	if (!isVisible()) {
		return;
	}

	Widget::render();

	ImageWidget::startBatch();

	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);

	for (int i=0; i < pictureCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
		}
	}

	for (int i=0; i < titleCellCount; ++i) {
		if (ImageWidget::getImage(i + pictureCellCount)) {
			ImageWidget::renderImage(i + pictureCellCount, light);
		}
	}

	for (int i=0; i < choicesCellCount; ++i) {
		if (ImageWidget::getImage(i + pictureCellCount + titleCellCount)) {
			ImageWidget::renderImage(i + pictureCellCount + titleCellCount, light);
		}
	}

	ImageWidget::endBatch();

	if (!TextWidget::getText(0).empty()) {
		TextWidget::renderTextShadowed(0);
	}
	if (!TextWidget::getText(1).empty()) {
		TextWidget::renderTextShadowed(1);
	}
	if (!TextWidget::getText(2).empty()) {
		TextWidget::renderTextShadowed(2);
	}
	if (!TextWidget::getText(3).empty()) {
		TextWidget::renderTextShadowed(3);
	}
	if (!TextWidget::getText(4).empty()) {
		TextWidget::renderTextShadowed(4);
	}
	if (!TextWidget::getText(5).empty()) {
		TextWidget::renderTextShadowed(5);
	}
	if (!TextWidget::getText(6).empty()) {
		TextWidget::renderTextShadowed(6);
	}
	if (!TextWidget::getText(7).empty()) {
		TextWidget::renderTextShadowed(7);
	}
	if (!TextWidget::getText(8).empty()) {
		TextWidget::renderTextShadowed(8);
	}
}

void EventWindow::computePicturePanel() {
    if (m_ui->getEventCount() > 0) {
        Event *event = m_ui->getEvent(0);
    }
}

void EventWindow::computeTitlePanel() {
    if (m_ui->getEventCount() > 0) {
        Event *event = m_ui->getEvent(0);
        string title = event->getType()->getName();
        TextWidget::setText(title, 0);
    }
}

void EventWindow::computeChoicesPanel() {
    if (m_ui->getEventCount() > 0) {
        Event *event = m_ui->getEvent(0);
        for (int i = 0; i < event->getType()->getChoiceCount(); ++i) {
            const Choice *choice = event->getType()->getChoice(i);
            const Texture2D *image = choice->getFlavorImage();
            string title = choice->getFlavorTitle();
            TextWidget::setText(title, i+1);
            setDownImage(i*6 + pictureCellCount + titleCellCount, image);
        }
    }
}

void EventWindow::computeChoicesInfo(int posDisplay) {
    int displayPos = 0;
    if (posDisplay < 6 && posDisplay > -1) {
        displayPos = 0;
    } else if (posDisplay >= 6 && posDisplay < 12) {
        displayPos = 1;
    } else if (posDisplay >= 12 && posDisplay < 18) {
        displayPos = 2;
    } else if (posDisplay >= 18 && posDisplay < 24) {
        displayPos = 3;
    } else if (posDisplay >= 24 && posDisplay < 30) {
        displayPos = 4;
    } else if (posDisplay >= 30 && posDisplay < 36) {
        displayPos = 5;
    } else if (posDisplay >= 36 && posDisplay < 42) {
        displayPos = 6;
    } else if (posDisplay >= 42 && posDisplay < 48) {
        displayPos = 7;
    }
    if (m_ui->getEventCount() > 0) {
        Lang &lang = g_lang;
        stringstream ss;
        Event *event = m_ui->getEvent(0);
        Choice *choice = event->getType()->getChoice(displayPos);
        string flavorTitle = choice->getFlavorTitle();
        string desc = "";
        choice->getDesc(desc, "\n");
        ss << desc;
        setToolTipText2(flavorTitle, ss.str(), EventDisplaySection::CHOICES);
    }
}

void EventWindow::eventButtonPressed(int posDisplay) {
    int displayPos = 0;
    if (posDisplay < 6 && posDisplay > -1) {
        displayPos = 0;
    } else if (posDisplay >= 6 && posDisplay < 12) {
        displayPos = 1;
    } else if (posDisplay >= 12 && posDisplay < 18) {
        displayPos = 2;
    } else if (posDisplay >= 18 && posDisplay < 24) {
        displayPos = 3;
    } else if (posDisplay >= 24 && posDisplay < 30) {
        displayPos = 4;
    } else if (posDisplay >= 30 && posDisplay < 36) {
        displayPos = 5;
    } else if (posDisplay >= 36 && posDisplay < 42) {
        displayPos = 6;
    } else if (posDisplay >= 42 && posDisplay < 48) {
        displayPos = 7;
    }
    Event *event = m_ui->getEvent(0);
    Choice *choice = event->getType()->getChoice(displayPos);
    choice->processChoice(event->getFaction(), event->getTargetList());
    m_ui->removeEvent();
    clear();
    setVisible(false);
    m_ui->computeDisplay();
    if (m_ui->getEventCount() == 0) {
        g_simInterface.resume();
    }
}

EventDisplayButton EventWindow::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[3] = { m_pictureOffset, m_titleOffset, m_choicesOffset };
	int counts[3] = { pictureCellCount, titleCellCount, choicesCellCount };
	for (int i=0; i < 3; ++i) {
		pos = i_pos - offsets[i];
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
					return EventDisplayButton(EventDisplaySection(i), index);
				}
				return EventDisplayButton(EventDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return EventDisplayButton(EventDisplaySection::INVALID, invalidIndex);
}

void EventWindow::setToolTipText2(const string &hdr, const string &tip, EventDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == EventDisplaySection::CHOICES) {
		a_offset = m_choicesOffset;
	} else {

	}
	resetTipPos(a_offset);
}

void EventWindow::tick() {
    EventDisplayButton prodBtn = getHoverButton();
	if (prodBtn.m_section == EventDisplaySection::CHOICES) {
        computeChoicesInfo(prodBtn.m_index);
	}
}

bool EventWindow::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == EventDisplaySection::CHOICES) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else {
				m_pressedBtn = EventDisplayButton(EventDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return false;
}

bool EventWindow::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != EventDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == EventDisplaySection::CHOICES) {
						eventButtonPressed(m_hoverBtn.m_index);
					}
					m_pressedBtn = EventDisplayButton(EventDisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
		}
	}
	return false;
}

bool EventWindow::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void EventWindow::resetTipPos(Vec2i i_offset) {
	if (m_toolTip->isEmpty()) {
		m_toolTip->setVisible(false);
		return;
	}
	Vec2i ctPos = getScreenPos() + i_offset;
	if (ctPos.x > g_metrics.getScreenW() / 2) {
		ctPos.x -= (m_toolTip->getWidth() + getBorderLeft() + 3);
	} else {
		ctPos.x += (getWidth() - getBorderRight() + 3);
	}
	if (ctPos.y + m_toolTip->getHeight() > g_metrics.getScreenH()) {
		int offset = g_metrics.getScreenH() - (ctPos.y + m_toolTip->getHeight());
		ctPos.y += offset;
	}
	m_toolTip->setPos(ctPos);
	m_toolTip->setVisible(true);
}

ostream& operator<<(ostream &stream, const EventDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool EventWindow::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		EventDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == EventDisplaySection::CHOICES) {
                computeChoicesInfo(currBtn.m_index);
			} else {
                setToolTipText2("", "", EventDisplaySection::INVALID);
            }
			m_hoverBtn = currBtn;
			return true;
		}
	} else {
		setToolTipText2("", "", EventDisplaySection::INVALID);
	}
	return false;
}

void EventWindow::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = EventDisplayButton(EventDisplaySection::INVALID, invalidIndex);
    setToolTipText2("", "", EventDisplaySection::INVALID);
	m_ui->invalidateActivePos();
}

}}
