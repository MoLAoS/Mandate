// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "production_display.h"

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
//  class ProductionDisplayFrame
// =====================================================

ProductionDisplayFrame::ProductionDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::CLOSE | ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new ProductionWindow(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &ProductionDisplayFrame::onExpand);
	Shrink.connect(this, &ProductionDisplayFrame::onShrink);
	Close.connect(this, &ProductionDisplayFrame::remove);
	setPinned(g_config.getUiPinWidgets());
}

void ProductionDisplayFrame::remove(Widget*) {
    setVisible(false);
}

void ProductionDisplayFrame::resetSize() {
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

void ProductionDisplayFrame::onExpand(Widget*) {
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

void ProductionDisplayFrame::onShrink(Widget*) {
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

void ProductionDisplayFrame::render() {
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

void ProductionDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class ProductionWindow
// =====================================================

ProductionWindow::ProductionWindow(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(880, 5))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(ProductionDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(ProductionDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);

	for (int i = 0; i < resourceCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < itemCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < processCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < unitCellCount; ++i) {
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

void ProductionWindow::layout() {
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
	m_resourceOffset = Vec2i(x, y);
	for (int i = 0; i < resourceCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	x = 0;
	y += m_imageSize;
    m_itemOffset = Vec2i(x, y);
	for (int i = 0; i < itemCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, resourceCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

    x = 0;
    y += m_imageSize;
	m_processOffset = Vec2i(x, y);
	for (int i = 0; i < processCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, resourceCellCount + itemCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

    x = 0;
    y += m_imageSize;
	m_unitOffset = Vec2i(x, y);
	for (int i = 0; i < unitCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, resourceCellCount + itemCellCount + processCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

	for (int i = 0; i < resourceCellCount + itemCellCount + processCellCount + unitCellCount; ++i) {
		//setTextFont(fontIndex, i);
	}

	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void ProductionWindow::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastProductionDisplaySize(sz);
	cfg.setUiLastProductionDisplayPosX(pos.x);
	cfg.setUiLastProductionDisplayPosY(pos.y);
}

void ProductionWindow::reset() {
	Config &cfg = g_config;
	cfg.setUiLastProductionDisplaySize(2);
	cfg.setUiLastProductionDisplayPosX(-1);
	cfg.setUiLastProductionDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<ProductionDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<ProductionDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void ProductionWindow::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void ProductionWindow::setSize() {
	Vec2i sz = m_sizes.displaySize;
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<ProductionDisplayFrame*>(m_parent)->resetSize();
}

void ProductionWindow::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < resourceCellCount; ++i) {
		ImageWidget::setImage(0, i);
	}

	for (int i=0; i < itemCellCount; ++i) {
		ImageWidget::setImage(0, i + resourceCellCount);
	}

	for (int i=0; i < processCellCount; ++i) {
		ImageWidget::setImage(0, i + resourceCellCount + itemCellCount);
	}

    for (int i=0; i < unitCellCount; ++i) {
		ImageWidget::setImage(0, i + resourceCellCount + itemCellCount + processCellCount);
	}
}

void ProductionWindow::render() {
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
		}
	}

	for (int i=0; i < itemCellCount; ++i) {
		if (ImageWidget::getImage(i + resourceCellCount)) {
			ImageWidget::renderImage(i + resourceCellCount, light);
		}
	}

	for (int i=0; i < processCellCount; ++i) {
		if (ImageWidget::getImage(i + resourceCellCount + itemCellCount)) {
			ImageWidget::renderImage(i + resourceCellCount + itemCellCount, light);
		}
	}

	for (int i=0; i < unitCellCount; ++i) {
		if (ImageWidget::getImage(i + resourceCellCount + itemCellCount + processCellCount)) {
			ImageWidget::renderImage(i + resourceCellCount + itemCellCount + processCellCount, light);
		}
	}

	ImageWidget::endBatch();
}

void ProductionWindow::computeResourcesPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getType()->getResourceProductionSystem()->getCreatedResourceCount(); ++i) {
                const Texture2D *image = u->getType()->getResourceProductionSystem()->getCreatedResource(i, u->getFaction()).getType()->getImage();
                setDownImage(i, image);
            }
        }
    }
}

void ProductionWindow::computeResourcesInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            Lang &lang = g_lang;
            stringstream ss;
            ResourceAmount r = u->getType()->getResourceProductionSystem()->getCreatedResource(posDisplay, u->getFaction());
            string resName = lang.getTechString(r.getType()->getName());
            Timer tR = u->getType()->getResourceProductionSystem()->getCreatedResourceTimer(posDisplay, u->getFaction());
            int cStep = u->productionSystemTimers.currentSteps[posDisplay].currentStep;
            if (resName == r.getType()->getName()) {
                resName = formatString(resName);
            }
            ss << endl << lang.get("Create") << ": ";
            ss << r.getAmount() << " " << resName << " " << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
            setToolTipText2("Make Resource", ss.str(), ProductionDisplaySection::RESOURCES);
        }
    }
}

void ProductionWindow::computeItemPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getType()->getItemProductionSystem()->getCreatedItemCount(); ++i) {
                const Texture2D *image = u->getType()->getItemProductionSystem()->getCreatedItem(i, u->getFaction()).getType()->getImage();
                setDownImage(i + resourceCellCount, image);
            }
        }
    }
}

void ProductionWindow::computeItemInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            Lang &lang = g_lang;
            stringstream ss;
            CreatedItem cr = u->getType()->getItemProductionSystem()->getCreatedItem(posDisplay, u->getFaction());
            string resName = lang.getTechString(cr.getType()->getName());
            Timer tR = u->getType()->getItemProductionSystem()->getCreatedItemTimer(posDisplay, u->getFaction());
            int cStep = u->productionSystemTimers.currentItemSteps[posDisplay].currentStep;
            if (resName == cr.getType()->getName()) {
                resName = formatString(resName);
            }
            ss << endl << lang.get("Create") << ": ";
            ss << cr.getAmount() << " " << resName << " " << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
            setToolTipText2("Make Item", ss.str(), ProductionDisplaySection::ITEMS);
        }
    }
}

void ProductionWindow::computeProcessPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getType()->getProcessProductionSystem()->getProcessCount(); ++i) {
                const Texture2D *image = NULL;
                if (u->getType()->getProcessProductionSystem()->getProcess(i, u->getFaction()).products.size() > 0) {
                    image = u->getType()->getProcessProductionSystem()->getProcess(i, u->getFaction()).products[0].getType()->getImage();
                } else if (u->getType()->getProcessProductionSystem()->getProcess(i, u->getFaction()).items.size() > 0) {
                    image = u->getType()->getProcessProductionSystem()->getProcess(i, u->getFaction()).items[0].getType()->getImage();
                }
                setDownImage(i + resourceCellCount + itemCellCount, image);
            }
        }
    }
}

void ProductionWindow::computeProcessInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            Lang &lang = g_lang;
            stringstream ss;
            ss << endl << lang.get("Process") << ": ";
            Timer tR = u->getType()->getProcessProductionSystem()->getProcessTimer(posDisplay, u->getFaction());
            int cStep = u->productionSystemTimers.currentProcessSteps[posDisplay].currentStep;
            ss << endl << lang.get("Timer") << ": " << cStep << "/" << tR.getTimerValue();
            string scope;
            if (u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].getScope() == true) {
                scope = lang.get("local");
            } else {
                scope = lang.get("faction");
            }
            ss << endl << lang.get("Scope") << ": " << scope;
            ss << endl << lang.get("Costs") << ": ";
            for (int c = 0; c < u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].costs.size(); ++c) {
                const ResourceType *costsRT = u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].costs[c].getType();
                string resName = lang.getTechString(costsRT->getName());
                if (resName == costsRT->getName()) {
                    resName = formatString(resName);
                }
                ss << endl << u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].costs[c].getAmount() << " " << resName;
            }
            ss << endl << lang.get("Products") << ": ";
            for (int p = 0; p < u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].products.size(); ++p) {
                const ResourceType *productsRT = u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].products[p].getType();
                string resName = lang.getTechString(productsRT->getName());
                if (resName == productsRT->getName()) {
                    resName = formatString(resName);
                }
                ss << endl << u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].products[p].getAmount() << " " << resName;
            }
            ss << endl << lang.get("Items") << ": ";
            for (int t = 0; t < u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].items.size(); ++t) {
                const ItemType *itemsIT = u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].items[t].getType();
                string itemName = lang.getTechString(itemsIT->getName());
                if (itemName == itemsIT->getName()) {
                    itemName = formatString(itemName);
                }
                ss << endl << u->getType()->getProcessProductionSystem()->getProcesses()[posDisplay].items[t].getAmount() << " " << itemName;
            }
            setToolTipText2("Process", ss.str(), ProductionDisplaySection::PROCESSES);
        }
    }
}

void ProductionWindow::computeUnitPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getType()->getUnitProductionSystem()->getCreatedUnitCount(); ++i) {
                const Texture2D *image = u->getType()->getUnitProductionSystem()->getCreatedUnit(i, u->getFaction()).getType()->getImage();
                setDownImage(i + resourceCellCount + itemCellCount + processCellCount, image);
            }
        }
    }
}

void ProductionWindow::computeUnitInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        Unit *unit = 0;
        for (int k = 0; k < g_world.getFactionCount(); ++k) {
            Faction *faction = g_world.getFaction(k);
            for (int j = 0; j < faction->getUnitCount(); ++j) {
                if (faction->getUnit(j)->getId() == u->getId()) {
                    unit = faction->getUnit(j);
                }
            }
        }
        if (u->isBuilt()) {
            Lang &lang = g_lang;
            stringstream ss;
            CreatedUnit *cu = unit->getCreatedUnit(posDisplay);
            string resName = lang.getTechString(cu->getType()->getName());
            Timer *tR = unit->getCreatedUnitTimer(posDisplay);
            int cStep = unit->productionSystemTimers.currentUnitSteps[posDisplay].currentStep;
            if (resName == cu->getType()->getName()) {
                resName = formatString(resName);
            }
            ss << endl << lang.get("Create") << ": ";
            ss << cu->getAmount() << " " << resName << " " << lang.get("Timer") << ": " << cStep << "/" << tR->getTimerValue();
            setToolTipText2("Make Unit", ss.str(), ProductionDisplaySection::UNITS);
        }
    }
}

ProductionDisplayButton ProductionWindow::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[4] = { m_resourceOffset, m_itemOffset, m_processOffset, m_unitOffset };
	int counts[4] = { resourceCellCount, itemCellCount, processCellCount, unitCellCount };
	for (int i=0; i < 4; ++i) {
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
					return ProductionDisplayButton(ProductionDisplaySection(i), index);
				}
				return ProductionDisplayButton(ProductionDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return ProductionDisplayButton(ProductionDisplaySection::INVALID, invalidIndex);
}

void ProductionWindow::setToolTipText2(const string &hdr, const string &tip, ProductionDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == ProductionDisplaySection::RESOURCES) {
		a_offset = m_resourceOffset;
	} else if (i_section == ProductionDisplaySection::ITEMS) {
		a_offset = m_itemOffset;
	} else if (i_section == ProductionDisplaySection::PROCESSES) {
		a_offset = m_processOffset;
	} else if (i_section == ProductionDisplaySection::UNITS) {
		a_offset = m_unitOffset;
	} else {

	}
	resetTipPos(a_offset);
}

void ProductionWindow::tick() {
    ProductionDisplayButton prodBtn = getHoverButton();
	if (prodBtn.m_section == ProductionDisplaySection::RESOURCES) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeResourcesInfo(prodBtn.m_index);
		}
	}
	if (prodBtn.m_section == ProductionDisplaySection::ITEMS) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeItemInfo(prodBtn.m_index);
		}
	}
	if (prodBtn.m_section == ProductionDisplaySection::PROCESSES) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeProcessInfo(prodBtn.m_index);
		}
	}
	if (prodBtn.m_section == ProductionDisplaySection::UNITS) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeUnitInfo(prodBtn.m_index);
		}
	}
}

bool ProductionWindow::mouseDown(MouseButton btn, Vec2i pos) {
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

bool ProductionWindow::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != ProductionDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
			}
		}
	}
	return false;
}

bool ProductionWindow::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void ProductionWindow::resetTipPos(Vec2i i_offset) {
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

ostream& operator<<(ostream &stream, const ProductionDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool ProductionWindow::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		ProductionDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == ProductionDisplaySection::RESOURCES) {
                computeResourcesInfo(currBtn.m_index);
			} else if (currBtn.m_section == ProductionDisplaySection::ITEMS) {
                computeItemInfo(currBtn.m_index);
			} else if (currBtn.m_section == ProductionDisplaySection::PROCESSES) {
                computeProcessInfo(currBtn.m_index);
			} else if (currBtn.m_section == ProductionDisplaySection::UNITS) {
                computeUnitInfo(currBtn.m_index);
			} else {
                setToolTipText2("", "", ProductionDisplaySection::INVALID);
            }
			m_hoverBtn = currBtn;
			return true;
		}
	} else {
		setToolTipText2("", "", ProductionDisplaySection::INVALID);
	}
	return false;
}

void ProductionWindow::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = ProductionDisplayButton(ProductionDisplaySection::INVALID, invalidIndex);
    setToolTipText2("", "", ProductionDisplaySection::INVALID);
	m_ui->invalidateActivePos();
}

}}
