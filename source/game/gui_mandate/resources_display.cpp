// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "resources_display.h"

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
//  class ResourcesDisplayFrame
// =====================================================

ResourcesDisplayFrame::ResourcesDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::CLOSE | ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new ResourcesWindow(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &ResourcesDisplayFrame::onExpand);
	Shrink.connect(this, &ResourcesDisplayFrame::onShrink);
	Close.connect(this, &ResourcesDisplayFrame::remove);
	setPinned(g_config.getUiPinWidgets());
}

void ResourcesDisplayFrame::remove(Widget*) {
    setVisible(false);
}

void ResourcesDisplayFrame::resetSize() {
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

void ResourcesDisplayFrame::onExpand(Widget*) {
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

void ResourcesDisplayFrame::onShrink(Widget*) {
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

void ResourcesDisplayFrame::render() {
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

void ResourcesDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class ResourcesWindow
// =====================================================

ResourcesWindow::ResourcesWindow(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(880, 5))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(ResourcesDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(ResourcesDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::RESOURCES_DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);

	for (int i = 0; i < resourceCellCount; ++i) {
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

void ResourcesWindow::layout() {
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
	m_sizes.displaySize = Vec2i(m_imageSize * 6, m_imageSize * 17);

	x = 0;
	y = 0;
	m_resourceOffset = Vec2i(x, y);
	for (int i = 0; i < resourceCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		TextWidget::addText("");
		TextWidget::setTextPos(Vec2i(x, y + m_imageSize / 2), i);
		x += m_imageSize;
	}

	for (int i = 0; i < resourceCellCount; ++i) {
		setTextFont(fontIndex, i);
	}

	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void ResourcesWindow::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastResourcesDisplaySize(sz);
	cfg.setUiLastResourcesDisplayPosX(pos.x);
	cfg.setUiLastResourcesDisplayPosY(pos.y);
}

void ResourcesWindow::reset() {
	Config &cfg = g_config;
	cfg.setUiLastResourcesDisplaySize(2);
	cfg.setUiLastResourcesDisplayPosX(-1);
	cfg.setUiLastResourcesDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<ResourcesDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<ResourcesDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void ResourcesWindow::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void ResourcesWindow::setSize() {
	Vec2i sz = m_sizes.displaySize;
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<ResourcesDisplayFrame*>(m_parent)->resetSize();
}

void ResourcesWindow::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < resourceCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}
}

void ResourcesWindow::render() {
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

	for (int i=0; i < resourceCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, light);
            if (!TextWidget::getText(i).empty()) {
                TextWidget::renderTextShadowed(i);
            }
		}
	}

	ImageWidget::endBatch();
}

void ResourcesWindow::computeStoragePanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getType()->getResourceProductionSystem()->getStoredResourceCount(); ++i) {
                const Texture2D *image = u->getType()->getResourceProductionSystem()->getStoredResource(i, u->getFaction()).getType()->getImage();
                setDownImage(i, image);

                const ResourceType *rt = u->getType()->getResourceProductionSystem()->getStoredResource(i, u->getFaction()).getType();
                int stored = u->getSResource(rt)->getAmount();
                stringstream ss;
                if (stored > 9999) {
                    stored = stored / 1000;
                    ss << stored;
                    ss << "k";
                } else {
                    ss << stored;
                }
                string text = ss.str();
                TextWidget::setText(text, i);
            }
        }
    }
}

void ResourcesWindow::computeStorageInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            Lang &lang = g_lang;
            stringstream ss;
            const ResourceType *rt = u->getType()->getResourceProductionSystem()->getStoredResource(posDisplay, u->getFaction()).getType();
            ss << rt->getName() << ": " << u->getSResource(rt)->getAmount() << "/"
            << u->getType()->getResourceProductionSystem()->getStoredResource(posDisplay, u->getFaction()).getAmount();
            setToolTipText2("Stored:", ss.str(), ResourcesDisplaySection::RESOURCES);
        }
    }
}

ResourcesDisplayButton ResourcesWindow::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[1] = { m_resourceOffset };
	int counts[1] = { resourceCellCount };
	for (int i=0; i < 1; ++i) {
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
					return ResourcesDisplayButton(ResourcesDisplaySection(i), index);
				}
				return ResourcesDisplayButton(ResourcesDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return ResourcesDisplayButton(ResourcesDisplaySection::INVALID, invalidIndex);
}

void ResourcesWindow::setToolTipText2(const string &hdr, const string &tip, ResourcesDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == ResourcesDisplaySection::RESOURCES) {
		a_offset = m_resourceOffset;
	} else {

	}
	resetTipPos(a_offset);
}

void ResourcesWindow::tick() {
    ResourcesDisplayButton prodBtn = getHoverButton();
	if (prodBtn.m_section == ResourcesDisplaySection::RESOURCES) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeStorageInfo(prodBtn.m_index);
		}
	}
}

bool ResourcesWindow::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
		}
	}
	return false;
}

bool ResourcesWindow::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != ResourcesDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
			}
		}
	}
	return false;
}

bool ResourcesWindow::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void ResourcesWindow::resetTipPos(Vec2i i_offset) {
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

ostream& operator<<(ostream &stream, const ResourcesDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool ResourcesWindow::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		ResourcesDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == ResourcesDisplaySection::RESOURCES) {
                computeStorageInfo(currBtn.m_index);
			} else {
                setToolTipText2("", "", ResourcesDisplaySection::INVALID);
            }
			m_hoverBtn = currBtn;
			return true;
		}
	} else {
		setToolTipText2("", "", ResourcesDisplaySection::INVALID);
	}
	return false;
}

void ResourcesWindow::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = ResourcesDisplayButton(ResourcesDisplaySection::INVALID, invalidIndex);
    setToolTipText2("", "", ResourcesDisplaySection::INVALID);
	m_ui->invalidateActivePos();
}

}}
