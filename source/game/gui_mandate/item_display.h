// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
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

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace ProtoTypes;
using Entities::Faction;

class UserInterface;

WRAPPED_ENUM( ItemDisplaySection, SELECTION, BUTTONS, EQUIPMENT, INVENTORY, DESCRIPTION )

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
	static const int selectionCellCount = cellWidthCount * cellHeightCount * 4;
	static const int buttonCellCount = cellWidthCount * cellHeightCount;
	static const int equipmentCellCount = cellWidthCount * cellHeightCount;
	static const int inventoryCellCount = cellWidthCount * cellHeightCount;
	static const int descriptionCellCount = cellWidthCount * cellHeightCount;
	static const int invalidIndex = -1;
private:
	UserInterface *m_ui;
	bool downLighted[8];
	int index[1];
	int m_selectedCommandIndex;
	int m_logo;
	int m_imageSize;
	struct SizeItemCollection {
		Vec2i portraitSize;
		Vec2i logoSize;
		Vec2i commandSize;
	};
	SizeItemCollection m_sizes;
	FuzzySize m_fuzzySize;
	Vec2i m_portraitOffset,
	m_commandOffset;
	ItemDisplayButton m_hoverBtn,
                      m_pressedBtn;
	CommandTip *m_toolTip;
	const FontMetrics *m_fontMetrics;
private:
	void layout();
public:
	ItemWindow(Container *parent, UserInterface *ui, Vec2i pos);

	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	string getPortraitTitle() const					{return TextWidget::getText(0);}
	string getPortraitText() const					{return TextWidget::getText(1);}
	string getHelmetLabel() const				    {return TextWidget::getText(2);}
	string getPauldronsLabel() const				{return TextWidget::getText(3);}
	string getCuirassLabel() const				    {return TextWidget::getText(4);}
	string getGuantletsLabel() const				{return TextWidget::getText(5);}
	string getRightHandLabel() const				{return TextWidget::getText(6);}
	string getLeftHandLabel() const				    {return TextWidget::getText(7);}
	string getGreavesLabel() const				    {return TextWidget::getText(8);}
	string getBootsLabel() const				    {return TextWidget::getText(9);}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	int getSelectedCommandIndex() const             {return m_selectedCommandIndex;}
	CommandTip* getCommandTip()                     {return m_toolTip;}

	void setSize();
	void setFuzzySize(FuzzySize fSize);
	void setPortraitTitle(const string title);
	void setPortraitText(const string &text);
	void setHelmetText(const string &text);
	void setPauldronsText(const string &text);
	void setCuirassText(const string &text);
	void setGuantletsText(const string &text);
	void setRightHandText(const string &text);
	void setLeftHandText(const string &text);
	void setGreavesText(const string &text);
	void setBootsText(const string &text);

	void setHelmetLabel(bool v);
	void setPauldronsLabel(bool v);
	void setCuirassLabel(bool v);
	void setGauntletsLabel(bool v);
	void setRightHandLabel(bool v);
	void setLeftHandLabel(bool v);
	void setGreavesLabel(bool v);
	void setBootsLabel(bool v);

	void setUpImage(int i, const Texture2D *image) 		        {setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	        {setImage(image, i);}
	void setDownLighted(int i, bool lighted)			        {downLighted[i] = lighted;}
	void setIndex(int i, int value)						        {index[i] = value;}
	void setSelectedCommandPos(int i);

    void computeSelectionPanel();
	void computeButtonsPanel();
	void computeEquipmentPanel();


	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_commandOffset);}
	ItemDisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	ItemDisplayButton getHoverButton() const { return m_hoverBtn; }
	void persist();
	void reset();

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

	void resetSize();

	virtual void render() override;
	void setPinned(bool v);
};

}}//end namespace

#endif
