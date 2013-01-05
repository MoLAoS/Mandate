// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "item_display.h"

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
//  class ItemDisplayFrame
// =====================================================

ItemDisplayFrame::ItemDisplayFrame(UserInterface *ui, Vec2i pos)
		: Frame((Container*)WidgetWindow::getInstance(), ButtonFlags::SHRINK | ButtonFlags::EXPAND)
		, m_display(0)
		, m_ui(ui) {
	m_ui = ui;
	setWidgetStyle(WidgetType::GAME_WIDGET_FRAME);
	Frame::setTitleBarSize(20);

	m_display = new ItemWindow(this, ui, Vec2i(0,0));
	CellStrip::addCells(1);
	m_display->setCell(1);
	Anchors a(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::NONE, 0), Anchor(AnchorType::NONE, 0));
	m_display->setAnchors(a);
	setPos(pos);

	m_titleBar->enableShrinkExpand(false, true);
	Expand.connect(this, &ItemDisplayFrame::onExpand);
	Shrink.connect(this, &ItemDisplayFrame::onShrink);
	setPinned(g_config.getUiPinWidgets());
}

void ItemDisplayFrame::resetSize() {
	if (m_display->isVisible()) {
		if (!isVisible()) {
			setVisible(true);
		}
		Vec2i size = m_display->getSize() + getBordersAll() + Vec2i(0, 20);
		if (size != getSize()){
			setSize(size);
		}
	} else {
		setVisible(false);
	}
}

void ItemDisplayFrame::onExpand(Widget*) {
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

void ItemDisplayFrame::onShrink(Widget*) {
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

void ItemDisplayFrame::render() {
	if (m_ui->getSelection()->isEmpty() && !m_ui->getSelectedObject() && g_config.getUiPhotoMode()) {
		return;
	}
	Frame::render();
}

void ItemDisplayFrame::setPinned(bool v) {
	Frame::setPinned(v);
	m_titleBar->showShrinkExpand(!v);
}

// =====================================================
// 	class ItemWindow
// =====================================================

ItemWindow::ItemWindow(Container *parent, UserInterface *ui, Vec2i pos)
		: Widget(parent, pos, Vec2i(880, 5))
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_ui(ui)
		, m_logo(-1)
		, m_imageSize(32)
		, m_hoverBtn(ItemDisplaySection::INVALID, invalidIndex)
		, m_pressedBtn(ItemDisplaySection::INVALID, invalidIndex)
		, m_fuzzySize(FuzzySize::SMALL)
		, m_toolTip(0) {
	CHECK_HEAP();
	setWidgetStyle(WidgetType::DISPLAY);
	TextWidget::setAlignment(Alignment::NONE);

	for (int i = 0; i < buttonCellCount; ++i) {
		ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
        if (i < 1) {
            buttonNames[i] = "equip";
        } else {
            buttonNames[i] = "unequip";
        }
	}

	for (int i = 0; i < equipmentCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < inventoryCellCount; ++i) {
        ImageWidget::addImageX(0, Vec2i(0), Vec2i(m_imageSize));
	}

	for (int i = 0; i < resourcesCellCount; ++i) {
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

void ItemWindow::layout() {
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

	x = 0;
	y = 0;
	m_buttonOffset = Vec2i(x, y);
	for (int i = 0; i < buttonCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}
	m_sizes.commandSize = Vec2i(x, y+32+(m_imageSize*16));

	x = 0;
	y += m_imageSize;
    m_equipmentOffset = Vec2i(x, y);
	for (int i = 0; i < equipmentCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, buttonCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

    x = 0;
    y += m_imageSize;
	m_inventoryOffset = Vec2i(x, y);
	for (int i = 0; i < inventoryCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize;
			x = 0;
		}
		ImageWidget::setImageX(0, equipmentCellCount + buttonCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		x += m_imageSize;
	}

    x = 0;
    y += m_imageSize;
	m_resourcesOffset = Vec2i(x, y);
	for (int i = 0; i < resourcesCellCount; ++i) {
		if (i && i % cellWidthCount == 0) {
			y += m_imageSize * 2;
			x = 0;
		}
		ImageWidget::setImageX(0, equipmentCellCount + buttonCellCount + inventoryCellCount + i, Vec2i(x,y), Vec2i(m_imageSize));
		TextWidget::addText("");
		TextWidget::setTextPos(Vec2i(x, y + m_imageSize), i);
		x += m_imageSize;
	}

	for (int i = 0; i < resourcesCellCount; ++i) {
		setTextFont(fontIndex, i);
	}

	if (m_logo != invalidIndex) {
		ImageWidget::setImageX(0, m_logo, Vec2i(0, 0), Vec2i(m_imageSize * 6, m_imageSize * 6));
	}
}

void ItemWindow::persist() {
	Config &cfg = g_config;

	Vec2i pos = m_parent->getPos();
	int sz = getFuzzySize() + 1;

	cfg.setUiLastItemDisplaySize(sz);
	cfg.setUiLastItemDisplayPosX(pos.x);
	cfg.setUiLastItemDisplayPosY(pos.y);
}

void ItemWindow::reset() {
	Config &cfg = g_config;
	cfg.setUiLastItemDisplaySize(2);
	cfg.setUiLastItemDisplayPosX(-1);
	cfg.setUiLastItemDisplayPosY(-1);
	if (getFuzzySize() == FuzzySize::SMALL) {
		static_cast<ItemDisplayFrame*>(m_parent)->onExpand(0);
	} else if (getFuzzySize() == FuzzySize::LARGE) {
		static_cast<ItemDisplayFrame*>(m_parent)->onShrink(0);
	}
	m_parent->setPos(Vec2i(g_metrics.getScreenW() - 20 - m_parent->getWidth(), 20));
}

void ItemWindow::setFuzzySize(FuzzySize fuzzySize) {
	m_fuzzySize = fuzzySize;
	layout();
	setSize();
}

void ItemWindow::setSize() {
	Vec2i sz = m_sizes.commandSize;
	setVisible(true);
	Vec2i size = getSize();
	if (size != sz) {
		Widget::setSize(sz);
	}
	static_cast<ItemDisplayFrame*>(m_parent)->resetSize();
}

void ItemWindow::clear() {
	WIDGET_LOG( __FUNCTION__ << "()" );

	for (int i=0; i < buttonCellCount; ++i) {
		downLighted[i]= true;
		ImageWidget::setImage(0, i);
	}

	for (int i=0; i < equipmentCellCount; ++i) {
		downLighted[i + buttonCellCount]= true;
		ImageWidget::setImage(0, i + buttonCellCount);
	}

	for (int i=0; i < inventoryCellCount; ++i) {
		downLighted[i + buttonCellCount + equipmentCellCount]= true;
		ImageWidget::setImage(0, i + buttonCellCount + equipmentCellCount);
	}

    for (int i=0; i < resourcesCellCount; ++i) {
		ImageWidget::setImage(0, i + buttonCellCount + equipmentCellCount + inventoryCellCount);
	}
}

void ItemWindow::render() {
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

	for (int i=0; i < buttonCellCount; ++i) {
		if (ImageWidget::getImage(i)) {
			ImageWidget::renderImage(i, downLighted[i] ? light : dark);
		}
	}

	for (int i=0; i < equipmentCellCount; ++i) {
		if (ImageWidget::getImage(i + buttonCellCount)) {
			ImageWidget::renderImage(i + buttonCellCount, downLighted[i] ? light : dark);
		}
	}

	for (int i=0; i < inventoryCellCount; ++i) {
		if (ImageWidget::getImage(i + buttonCellCount + equipmentCellCount)) {
			ImageWidget::renderImage(i + buttonCellCount + equipmentCellCount, downLighted[i] ? light : dark);
		}
	}

	for (int i=0; i < resourcesCellCount; ++i) {
		if (ImageWidget::getImage(i + buttonCellCount + equipmentCellCount + inventoryCellCount)) {
			ImageWidget::renderImage(i + buttonCellCount + equipmentCellCount + inventoryCellCount, light);
            if (!TextWidget::getText(i).empty()) {
                TextWidget::renderTextShadowed(i);
            }
		}
	}

	ImageWidget::endBatch();
}

void ItemWindow::computeButtonsPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        const FactionType *ft = u->getFaction()->getType();
        if (u->isBuilt()) {
            for (int i = 0; i < 2; ++i) {
                setDownImage(i, ft->getItemImage(0));
                setDownLighted(i, true);
            }
        }
    }
}

void ItemWindow::computeButtonInfo(int posDisplay) {
    string name = buttonNames[posDisplay];
    if (equipping == true && posDisplay == 0) {
        name = name + " 0" + " equipping";
    }
    if (unequiping == true && posDisplay == 1) {
        name = name + " 1" + " unequiping";
    }
    setToolTipText2(name, "", ItemDisplaySection::BUTTONS);
}

void ItemWindow::equipButtonPressed(int posDisplay) {
    if (posDisplay == 0) {
        equipping = true;
        unequiping = false;
    } else if (posDisplay == 1) {
        unequiping = true;
        equipping = false;
    } else {

    }
}

void ItemWindow::computeEquipmentPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        const FactionType *ft = u->getFaction()->getType();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getType()->getEquipment().size(); ++i) {
                const Texture2D *image = ft->getItemImage(0);
                string name = u->getType()->getEquipment()[i].getTypeTag();
                if (u->getEquipment()[i].getCurrent() == 1) {
                    name = u->getEquipment()[i].getName();
                    for (int j = 0; j < u->getEquippedItems().size(); ++j) {
                        if (u->getEquipment()[i].getName() == u->getEquippedItem(j)->getType()->getName()) {
                            image = u->getEquippedItem(j)->getType()->getImage();
                            break;
                        }
                    }
                }
                setDownImage(i + buttonCellCount, image);
                setDownLighted(i + buttonCellCount, true);
            }
        }
    }
}

void ItemWindow::computeEquipmentInfo(int posDisplay) {
    const Unit *u = m_ui->getSelection()->getFrontUnit();
    string name = u->getType()->getEquipment()[posDisplay].getTypeTag();
    string body = "";
    if (u->getEquipment()[posDisplay].getCurrent() == 1) {
        name = u->getEquipment()[posDisplay].getName();
        int kind = 0;
        for (int i = 0; i < u->getEquipment().size(); ++i) {
            if (u->getEquipment()[i].getName() == name) {
                ++kind;
                if (i == posDisplay) {
                    break;
                }
            }
        }
        for (int i = 0; i < u->getEquippedItems().size(); ++i) {
            if (u->getEquippedItem(i)->getType()->getName() == name) {
                --kind;
                if (kind == 0) {
                    body = u->getEquippedItem(i)->getLongDesc();
                }
            }
        }
    }
    setToolTipText2(name, body, ItemDisplaySection::EQUIPMENT);
}

void ItemWindow::equipmentButtonPressed(int posDisplay) {
    if (unequiping == true) {
        if (m_ui->getSelection()->isComandable()) {
            const Unit *u = m_ui->getSelection()->getFrontUnit();
            Unit *unit = g_world.findUnitById(u->getId());
            for (int i = 0; i < u->getEquippedItems().size(); ++i) {
                if (u->getEquipment()[posDisplay].getName() == u->getEquippedItem(i)->getType()->getName()) {
                    unit->unequipItem(i);
                    break;
                }
            }
        }
    }
    unequiping = false;
}

void ItemWindow::inventoryButtonPressed(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        Unit *unit = g_world.findUnitById(u->getId());
        if (equipping == true) {
            for (int i = 0; i < u->getStoredItems().size(); ++i) {
                if (u->getStorage()[posDisplay].getName() == u->getStoredItem(i)->getType()->getName()) {
                    unit->equipItem(i);
                    break;
                }
            }
            equipping = false;
        } else if (u->getStorage()[posDisplay].getTypeTag() == "consumable") {
            for (int i = 0; i < u->getStoredItems().size(); ++i) {
                if (u->getStorage()[posDisplay].getName() == u->getStoredItem(i)->getType()->getName()) {
                    unit->consumeItem(i);
                    break;
                }
            }
        }
    }
}

void ItemWindow::computeInventoryPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            for (int i = 0; i < u->getStorage().size(); ++i) {
                if (i < 24) {
                const ItemType *type = u->getFaction()->getType()->getItemType(u->getStorage()[i].getName());
                setDownImage(i + buttonCellCount + equipmentCellCount, type->getImage());
                setDownLighted(i + buttonCellCount + equipmentCellCount, true);
                }
            }
        }
    }
}

void ItemWindow::computeInventoryInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        const ItemType *type = u->getFaction()->getType()->getItemType(u->getStorage()[posDisplay].getName());
        stringstream ss;
        ss << u->getStorage()[posDisplay].getCurrent();
        string name = type->getName()+ " (" + u->getStorage()[posDisplay].getTypeTag() + ") " + ": " + ss.str();
        setToolTipText2(name, "", ItemDisplaySection::INVENTORY);
    }
}

void ItemWindow::computeResourcesPanel() {
    if (m_ui->getSelection()->isComandable()) {
        const Unit *u = m_ui->getSelection()->getFrontUnit();
        if (u->isBuilt()) {
            int count = u->getType()->getResourceProductionSystem().getStoredResourceCount();
            for (int i = 0; i < resourcesCellCount; ++i) {
                if (i < resourcesCellCount && i < count) {
                    const Texture2D *image = u->getType()->getResourceProductionSystem().getStoredResource(i, u->getFaction()).getType()->getImage();
                    setDownImage(i + buttonCellCount + equipmentCellCount + inventoryCellCount, image);
                    const ResourceType *rt = u->getType()->getResourceProductionSystem().getStoredResource(i, u->getFaction()).getType();
                    int store = u->getType()->getResourceProductionSystem().getStoredResource(i, u->getFaction()).getAmount();
                    int stored = u->getSResource(rt)->getAmount();
                    string name = rt->getName();
                    stringstream ss;
                    ss << stored;
                    string text = ss.str();
                    TextWidget::setText(text, i);
                }
            }
        }
    }
}

void ItemWindow::computeResourcesInfo(int posDisplay) {
    if (m_ui->getSelection()->isComandable()) {
        /*const Unit *u = m_ui->getSelection()->getFrontUnit();
        stringstream ss;
        int displayPos = 0;
        if (posDisplay > -1 && posDisplay < 6) {
            displayPos = posDisplay;
        } else if (posDisplay > 5 && posDisplay < 12) {
            displayPos = posDisplay - 6;
        } else if (posDisplay > 11 && posDisplay < 18) {
            displayPos = posDisplay - 6;
        } else if (posDisplay > 17 && posDisplay < 24) {
            displayPos = posDisplay - 12;
        } else if (posDisplay > 23 && posDisplay < 30) {
            displayPos = posDisplay - 12;
        } else if (posDisplay > 29 && posDisplay < 36) {
            displayPos = posDisplay - 18;
        } else if (posDisplay > 35 && posDisplay < 42) {
            displayPos = posDisplay - 18;
        } else if (posDisplay > 41 && posDisplay < 48) {
            displayPos = posDisplay - 24;
        } else {
        }*/
        //const ResourceType *rt = u->getType()->getResourceProductionSystem().getStoredResource(displayPos, u->getFaction()).getType();;
        //ss << u->getSResource(rt)->getAmount();
        //string name = rt->getName() + ": " + ss.str();
        //setToolTipText2(name, "", ItemDisplaySection::RESOURCES);
    }
}

ItemDisplayButton ItemWindow::computeIndex(Vec2i i_pos, bool screenPos) {
	if (screenPos) {
		i_pos = i_pos - getScreenPos();
	}
	Vec2i pos = i_pos;
	Vec2i offsets[4] = { m_buttonOffset, m_equipmentOffset, m_inventoryOffset, m_resourcesOffset };
	int counts[4] = { buttonCellCount, equipmentCellCount, inventoryCellCount, resourcesCellCount };
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
					return ItemDisplayButton(ItemDisplaySection(i), index);
				} else if (i == 3) {
                    return ItemDisplayButton(ItemDisplaySection(i), index);
				}
				return ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
}

void ItemWindow::setToolTipText2(const string &hdr, const string &tip, ItemDisplaySection i_section) {
	m_toolTip->setHeader(hdr);
	m_toolTip->setTipText(tip);
	m_toolTip->clearItems();
	m_toolTip->setVisible(true);
	Vec2i a_offset;
	if (i_section == ItemDisplaySection::BUTTONS) {
		a_offset = m_buttonOffset;
	} else if (i_section == ItemDisplaySection::EQUIPMENT) {
		a_offset = m_equipmentOffset;
	} else if (i_section == ItemDisplaySection::INVENTORY) {
		a_offset = m_inventoryOffset;
	} else if (i_section == ItemDisplaySection::RESOURCES) {
		a_offset = m_resourcesOffset;
	} else {

	}
	resetTipPos(a_offset);
}

void ItemWindow::tick() {
    ItemDisplayButton itemBtn = getHoverButton();
	if (itemBtn.m_section == ItemDisplaySection::EQUIPMENT) {
        if (m_ui->getSelection()->getCount() == 1) {
			computeEquipmentInfo(itemBtn.m_index);
		}
	}
}

bool ItemWindow::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (Widget::isInsideBorders(pos)) {
			m_hoverBtn = computeIndex(pos, true);
			if (m_hoverBtn.m_section == ItemDisplaySection::BUTTONS) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else if (m_hoverBtn.m_section == ItemDisplaySection::EQUIPMENT) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else if (m_hoverBtn.m_section == ItemDisplaySection::INVENTORY) {
				m_pressedBtn = m_hoverBtn;
				return true;
			} else {
				m_pressedBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
			}
		}
	}
	return false;
}

bool ItemWindow::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (btn == MouseButton::LEFT) {
		if (m_pressedBtn.m_section != ItemDisplaySection::INVALID) {
			if (Widget::isInsideBorders(pos)) {
				m_hoverBtn = computeIndex(pos, true);
				if (m_hoverBtn == m_pressedBtn) {
					if (m_hoverBtn.m_section == ItemDisplaySection::BUTTONS) {
						equipButtonPressed(m_hoverBtn.m_index);
					} else if (m_hoverBtn.m_section == ItemDisplaySection::EQUIPMENT) {
                        equipmentButtonPressed(m_hoverBtn.m_index);
					} else if (m_hoverBtn.m_section == ItemDisplaySection::INVENTORY) {
                        inventoryButtonPressed(m_hoverBtn.m_index);
					} else if (m_hoverBtn.m_section == ItemDisplaySection::RESOURCES) {

					}
					m_pressedBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
					return true;
				}
			}
			m_pressedBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
		}
	}
	return false;
}

bool ItemWindow::mouseDoubleClick(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();

	if (Widget::isInsideBorders(pos)) {
		m_hoverBtn = computeIndex(pos, true);
        return false;
	}
	return mouseDown(btn, pos);
}

void ItemWindow::resetTipPos(Vec2i i_offset) {
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

ostream& operator<<(ostream &stream, const ItemDisplayButton &btn) {
	return stream << "Section: " << btn.m_section << " index: " << btn.m_index;
}

bool ItemWindow::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	Vec2i myPos = getScreenPos();
	Vec2i mySize = getSize();
	if (Widget::isInsideBorders(pos)) {
		ItemDisplayButton currBtn = computeIndex(pos, true);
		if (currBtn != m_hoverBtn) {
			if (currBtn.m_section == ItemDisplaySection::BUTTONS) {
                computeButtonInfo(currBtn.m_index);
			} else if (currBtn.m_section == ItemDisplaySection::EQUIPMENT) {
                computeEquipmentInfo(currBtn.m_index);
			} else if (currBtn.m_section == ItemDisplaySection::INVENTORY) {
                computeInventoryInfo(currBtn.m_index);
			} else if (currBtn.m_section == ItemDisplaySection::RESOURCES) {
                computeResourcesInfo(currBtn.m_index);
			} else {
                setToolTipText2("", "", ItemDisplaySection::INVALID);
            }
			m_hoverBtn = currBtn;
			return true;
		}
	} else {
		setToolTipText2("", "", ItemDisplaySection::INVALID);
	}
	return false;
}

void ItemWindow::mouseOut() {
	WIDGET_LOG( __FUNCTION__ << "()" );
	m_hoverBtn = ItemDisplayButton(ItemDisplaySection::INVALID, invalidIndex);
    setToolTipText2("", "", ItemDisplaySection::INVALID);
	m_ui->invalidateActivePos();
}

}}
