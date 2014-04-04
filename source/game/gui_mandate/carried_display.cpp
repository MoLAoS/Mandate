// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "carried_display.h"

#include "metrics.h"
#include "command_type.h"
#include "widget_window.h"
#include "core_data.h"
#include "user_interface.h"
#include "world.h"
#include "config.h"
#include "leak_dumper.h"

using namespace Shared::Graphics;

namespace Glest { namespace Gui {

using Global::CoreData;

// =====================================================
//  class CarriedDisplayFrame
// =====================================================

CarriedDisplayFrame::CarriedDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::CLOSE | ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new CarriedWindow(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &CarriedDisplayFrame::onExpand);
	Shrink.connect(this, &CarriedDisplayFrame::onShrink);
	Close.connect(this, &CarriedDisplayFrame::remove);
	setPinned(g_config.getUiPinWidgets());
}

void CarriedDisplayFrame::remove(Widget*) {
    setVisible(false);
}

void CarriedDisplayFrame::resetSize() {
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

void CarriedDisplayFrame::onExpand(Widget*) {
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

void CarriedDisplayFrame::onShrink(Widget*) {
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

void CarriedDisplayFrame::render() {
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && g_config.getUiPhotoMode()) {
		return;
	}
	if (!m_display->isVisible()) {
        setVisible(false);
	} else if (m_display->isVisible()) {
        setVisible(true);
	}
	Frame::render();
}

void CarriedDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class CarriedWindow
// =====================================================

CarriedWindow::CarriedWindow(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(880, 5))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(CarriedDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(CarriedDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);

	for (int i = 0; i < carriedCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < garrisonedCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < containedCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	const Texture2D* overlayImages[3] = {
		g_widgetConfig.getTickTexture(),
		g_widgetConfig.getCrossTexture(),
		g_widgetConfig.getQuestionTexture()
	};
	const Texture2D *logoTex = (g_world.getThisFaction()) ? g_world.getThisFaction()->getLogoTex() : 0;
	if (logoTex) {
		m_logo = ImageWidget::addImageX(logoTex, Vec2i(0, 0), Vec2i(192,192));
	}
	layout();
	clear();
	m_toolTip = new CommandTip(WidgetWindow::getInstance());
	m_toolTip->setVisible(false);
	CHECK_HEAP();
}

void CarriedWindow::layout() {
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

	m_sizes.logoSize = Vec2i(m_imageSize * 6, m_imageSize * 6);
	m_sizes.displaySize = Vec2i(m_imageSize * 6, m_imageSize*17);

	x = 0;
	y = 0;
	m_carriedOffset = Vec2i(x, y);
	for (int i = 0; i < carriedCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	x = 0;
	y += m_imageSize;
    m_garrisonedOffset = Vec2i(x, y);
	for (int i = 0; i < garrisonedCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, carriedCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

    x = 0;
    y += m_imageSize;
	m_containedOffset = Vec2i(x, y);
	for (int i = 0; i < containedCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, carriedCellCount + garrisonedCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	for (int i = 0; i < carriedCellCount + garrisonedCellCount + containedCellCount; ++i) {
		//setTextFont(fontIndex, i);
	}

	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void CarriedWindow::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastCarriedDisplaySize(sz);
	cfg.setUiLastCarriedDisplayPosX(pos.x);
	cfg.setUiLastCarriedDisplayPosY(pos.y);
}

void CarriedWindow::reset() {
	Config &cfg = g_config;
	cfg.setUiLastCarriedDisplaySize(2);
	cfg.setUiLastCarriedDisplayPosX(-1);
	cfg.setUiLastCarriedDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<CarriedDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<CarriedDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void CarriedWindow::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void CarriedWindow::setSize() {
	Vec2i sz = m_sizes.displaySize;
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<CarriedDisplayFrame*>(m_parent)->resetSize();
}

void CarriedWindow::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < carriedCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}

	for (int i=0; i < garrisonedCellCount; ++i) {
		ImageWidget::setImage(0, i + carriedCellCount);
	}

	for (int i=0; i < containedCellCount; ++i) {
		ImageWidget::setImage(0, i + carriedCellCount + garrisonedCellCount);
	}
}

void CarriedWindow::render() {
	if (!isVisible()) {
		return;
	}

	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject()) {
		if (g_config.getUiPhotoMode()) {
			return;
		}
		if (m_logo == -1) {
			setSize();
			RUNTIME_CHECK( !isVisible() );
			return;
		}
	}

	Widget::render();

	ImageWidget::startBatch();
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && m_logo != -1) {
		//ImageWidget::renderImage(m_logo);
	}

	Vec4f light(1.f), dark(0.3f, 0.3f, 0.3f, 1.f);

	for (int i=0; i < carriedCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
		}
	}

	for (int i=0; i < garrisonedCellCount; ++i) {
		if (ImageWidget::getImage(i + carriedCellCount)) {
			ImageWidget::renderImage(i + carriedCellCount, light);
		}
	}

	for (int i=0; i < containedCellCount; ++i) {
		if (ImageWidget::getImage(i + carriedCellCount + garrisonedCellCount)) {
			ImageWidget::renderImage(i + carriedCellCount + garrisonedCellCount, light);
		}
	}

	ImageWidget::endBatch();
}

void CarriedWindow::computeCarriedPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            bool transported = false;
            int i = 0;
            for (int ndx = 0; ndx < m_ui->getSelection()->getCount(); ++ndx) {
                if (m_ui->getSelection()->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
                    const Unit *unit = m_ui->getSelection()->getUnit(ndx);
                    UnitIdList carriedUnits = unit->getCarriedUnits();
                    if (!carriedUnits.empty()) {
                        transported = true;
                        foreach (UnitIdList, it, carriedUnits) {
                            if (i < carriedCellCount) {
                                Unit *unit = g_world.getUnit(*it);
                                setDownImage(i, unit->getType()->getImage());
                                ++i;
                            }
                        }
                    }
                }
            }
        }
    }
}

void CarriedWindow::computeCarriedInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            bool transported = false;
            int i = 0;
            if (u->isOfClass(UnitClass::CARRIER)) {
                UnitIdList carriedUnits = u->getCarriedUnits();
                if (!carriedUnits.empty()) {
                    transported = true;
                    foreach (UnitIdList, it, carriedUnits) {
                        if (i < carriedCellCount && i == posDisplay) {
                            Unit *unit = g_world.getUnit(*it);
                            string name = unit->getType()->getName();
                            string body = unit->getType()->getName();
                            body = unit->getLongDesc();
                            setToolTipText2(name, body, CarriedDisplaySection::CARRIED);
                        }
                        ++i;
                    }
                }
            }
        }
    }
}

void CarriedWindow::computeGarrisonedPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            bool transported = false;
            int i = 0;
            for (int ndx = 0; ndx < m_ui->getSelection()->getCount(); ++ndx) {
                if (m_ui->getSelection()->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
                    const Unit *unit = m_ui->getSelection()->getUnit(ndx);
                    UnitIdList garrisonedUnits = unit->getGarrisonedUnits();
                    if (!garrisonedUnits.empty()) {
                        transported = true;
                        foreach (UnitIdList, it, garrisonedUnits) {
                            if (i < Display::garrisonCellCount) {
                                Unit *unit = g_world.getUnit(*it);
                                setDownImage(i + carriedCellCount, unit->getType()->getImage());
                                ++i;
                            }
                        }
                    }
                }
            }
        }
    }
}

void CarriedWindow::computeGarrisonedInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            bool transported = false;
            int i = 0;
            for (int ndx = 0; ndx < m_ui->getSelection()->getCount(); ++ndx) {
                if (m_ui->getSelection()->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
                    const Unit *selUnit = m_ui->getSelection()->getUnit(ndx);
                    UnitIdList garrisonedUnits = selUnit->getGarrisonedUnits();
                    if (!garrisonedUnits.empty()) {
                        transported = true;
                        foreach (UnitIdList, it, garrisonedUnits) {
                            if (i < garrisonedCellCount) {
                                Unit *unit = g_world.getUnit(*it);
                                string name = unit->getType()->getName();
                                string body = unit->getType()->getName();
                                body = unit->getLongDesc();
                                setToolTipText2(name, body, CarriedDisplaySection::GARRISONED);
                                ++i;
                            }
                        }
                    }
                }
            }
        }
    }
}

void CarriedWindow::computeContainedPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            bool transported = false;
            int i = 0;
            for (int ndx = 0; ndx < m_ui->getSelection()->getCount(); ++ndx) {
                if (m_ui->getSelection()->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
                    const Unit *unit = m_ui->getSelection()->getUnit(ndx);
                    UnitIdList containedUnits = unit->getFaction()->getTransitingUnits();
                    if (!containedUnits.empty()) {
                        transported = true;
                        foreach (UnitIdList, it, containedUnits) {
                            if (i < containedCellCount) {
                                Unit *unit = g_world.getUnit(*it);
                                setDownImage(i + carriedCellCount + garrisonedCellCount, unit->getType()->getImage());
                                ++i;
                            }
                        }
                    }
                }
            }
        }
    }
}

void CarriedWindow::computeContainedInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            /*bool transported = false;
            int i = 0;
            for (int ndx = 0; ndx < m_ui->getSelection()->getCount(); ++ndx) {
                if (m_ui->getSelection()->getUnit(ndx)->getType()->isOfClass(UnitClass::CARRIER)) {
                    const Unit *selUnit = m_ui->getSelection()->getUnit(ndx);
                    UnitIdList containedUnits = selUnit->getContainedUnits();
                    if (!containedUnits.empty()) {
                        transported = true;
                        foreach (UnitIdList, it, containedUnits) {
                            if (i < containedCellCount) {
                                Unit *unit = g_world.getUnit(*it);
                                string name = unit->getType()->getName();
                                string body = unit->getType()->getName();
                                body = unit->getLongDesc();
                                setToolTipText2(name, body, CarriedDisplaySection::CONTAINED);
                                ++i;
                            }
                        }
                    }
                }
            }*/
        }
    }
}

CarriedDisplayButton CarriedWindow::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[3] = { m_carriedOffset, m_garrisonedOffset, m_containedOffset };
	int counts[3] = { carriedCellCount, garrisonedCellCount, containedCellCount };
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
					return CarriedDisplayButton(CarriedDisplaySection(i), index);
				}
				return CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
}

void CarriedWindow::setToolTipText2(const string &hdr, const string &tip, CarriedDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == CarriedDisplaySection::CARRIED) {
		a_offset = m_carriedOffset;
	} else if (i_section == CarriedDisplaySection::GARRISONED) {
		a_offset = m_garrisonedOffset;
	} else if (i_section == CarriedDisplaySection::CONTAINED) {
		a_offset = m_containedOffset;
	} else {

	}
	resetTipPos(a_offset);
}

void CarriedWindow::tick() {
    CarriedDisplayButton prodBtn = getHoverButton();
	if (prodBtn.m_section == CarriedDisplaySection::CARRIED) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeCarriedInfo(prodBtn.m_index);
		}
	}
	if (prodBtn.m_section == CarriedDisplaySection::GARRISONED) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeGarrisonedInfo(prodBtn.m_index);
		}
	}
	if (prodBtn.m_section == CarriedDisplaySection::CONTAINED) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeContainedInfo(prodBtn.m_index);
		}
	}
}

bool CarriedWindow::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == CarriedDisplaySection::CARRIED) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else if (m_hoverBtn.m_section == CarriedDisplaySection::GARRISONED) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else if (m_hoverBtn.m_section == CarriedDisplaySection::CONTAINED) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else {
				m_pressedBtn = CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return false;
}

bool CarriedWindow::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != CarriedDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
			}
		}
	}
	return false;
}

bool CarriedWindow::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
		if (m_hoverBtn.m_section == CarriedDisplaySection::CARRIED) {
			m_ui->unloadRequest(m_hoverBtn.m_index);
			m_hoverBtn = CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
			return true;
		} else if (m_hoverBtn.m_section == CarriedDisplaySection::GARRISONED) {
		    m_ui->degarrisonRequest(m_hoverBtn.m_index);
			m_hoverBtn = CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
			return true;
		} else if (m_hoverBtn.m_section == CarriedDisplaySection::CONTAINED) {
		    m_ui->factionUnloadRequest(m_hoverBtn.m_index);
			m_hoverBtn = CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
			return true;
		} else {
		    return false;
		}
	}
	return mouseDown(btn, pos);
}

void CarriedWindow::resetTipPos(Vec2i i_offset) {
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

ostream& operator<<(ostream &stream, const CarriedDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool CarriedWindow::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		CarriedDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == CarriedDisplaySection::CARRIED) {
                computeCarriedInfo(currBtn.m_index);
			} else if (currBtn.m_section == CarriedDisplaySection::GARRISONED) {
                computeGarrisonedInfo(currBtn.m_index);
			} else if (currBtn.m_section == CarriedDisplaySection::CONTAINED) {
                computeContainedInfo(currBtn.m_index);
			} else {
                setToolTipText2("", "", CarriedDisplaySection::INVALID);
            }
			m_hoverBtn = currBtn;
			return true;
		}
	} else {
		setToolTipText2("", "", CarriedDisplaySection::INVALID);
	}
	return false;
}

void CarriedWindow::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = CarriedDisplayButton(CarriedDisplaySection::INVALID, invalidIndex);
    setToolTipText2("", "", CarriedDisplaySection::INVALID);
	m_ui->invalidateActivePos();
}

}}
