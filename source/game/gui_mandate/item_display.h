// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_ITEM_DISPLAY_H_
#define _GLEST_GAME_ITEM_DISPLAY_H_

#include <string>
#include "texture.h"
#include "util.h"
#include "game_util.h"
#include "metrics.h"
#include "widgets.h"
#include "forward_decs.h"
#include "selection.h"
#include "framed_widgets.h"
#include "tool_tips.h"

using std::string;

using Shared::Graphics::Texture2D;
using namespace Shared::Math;
using Shared::Util::replaceBy;
using Glest::Global::Metrics;
using namespace Glest::Entities;

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace ProtoTypes;
using Entities::Faction;

class UserInterface;

WRAPPED_ENUM( ItemDisplaySection, BUTTONS, EQUIPMENT, INVENTORY, REQUISITION )

struct ItemDisplayButton {
	ItemDisplaySection m_section;
	int m_index;

	ItemDisplayButton(ItemDisplaySection s, int ndx) : m_section(s), m_index(ndx) {}

	ItemDisplayButton& operator=(const ItemDisplayButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const ItemDisplayButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const ItemDisplayButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class ItemWindow
//
///	Display for items
// =====================================================

class ItemWindow : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 1;
	static const int buttonCellCount = cellWidthCount * cellHeightCount;
	static const int equipmentCellCount = cellWidthCount * cellHeightCount * 3;
	static const int inventoryCellCount = cellWidthCount * cellHeightCount * 3;
	static const int requisitionCellCount = cellWidthCount * cellHeightCount * 3;
	static const int invalidIndex = -1;
private:
	UserInterface *m_ui;
	bool downLighted[buttonCellCount + equipmentCellCount + inventoryCellCount + requisitionCellCount];
	int index[1];
	int m_logo;
	int m_imageSize;
	struct SizeItemCollection {
		Vec2i logoSize;
		Vec2i commandSize;
	};
	SizeItemCollection m_sizes;
	FuzzySize m_fuzzySize;
	Vec2i m_buttonOffset,
	m_equipmentOffset,
	m_inventoryOffset,
	m_requisitionsOffset;

	ItemDisplayButton m_hoverBtn,
                      m_pressedBtn;

	CommandTip *m_toolTip;
	const FontMetrics *m_fontMetrics;

	string buttonNames[buttonCellCount];

	bool equipping;
	bool unequiping;

private:
	void layout();
public:
	ItemWindow(Container *parent, UserInterface *ui, Vec2i pos);

	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	CommandTip* getCommandTip()                     {return m_toolTip;}

	void setSize();
	void setFuzzySize(FuzzySize fSize);

	void setUpImage(int i, const Texture2D *image) 		        {setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	        {setImage(image, i);}
	void setDownLighted(int i, bool lighted)			        {downLighted[i] = lighted;}
	void setIndex(int i, int value)						        {index[i] = value;}

	void tick();

	void computeButtonsPanel();
	void computeButtonInfo(int posDisplay);
	void equipButtonPressed(int posDisplay);
	void equipmentButtonPressed(int posDisplay);
	void inventoryButtonPressed(int posDisplay);
	void requisitionButtonPressed(int posDisplay);
	void computeEquipmentPanel();
	void computeEquipmentInfo(int posDisplay);
	void computeInventoryPanel();
	void computeInventoryInfo(int posDisplay);
	void computeRequisitionPanel();
	void computeRequisitionInfo(int posDisplay);

	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_buttonOffset);}
	ItemDisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	ItemDisplayButton getHoverButton() const { return m_hoverBtn; }
	void persist();
	void reset();

	void setToolTipText2(const string &hdr, const string &tip, ItemDisplaySection i_section);

	virtual void render() override;
	virtual string descType() const override { return "DisplayPanel"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual void mouseOut() override;
};

// =====================================================
//  class ItemDisplayFrame
// =====================================================

class ItemDisplayFrame : public Frame {
private:
	ItemWindow *m_display;
	UserInterface *m_ui;

public:
	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	ItemDisplayFrame(UserInterface *ui, Vec2i pos);
	ItemWindow* getItemDisplay() {return m_display;}

	void remove(Widget*);
	void resetSize();

	virtual void render() override;
	void setPinned(bool v);
};

}}//end namespace

#endif
