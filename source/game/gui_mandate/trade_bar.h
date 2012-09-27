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

#ifndef _GLEST_GAME_TRADE_BAR_H_
#define _GLEST_GAME_TRADE_BAR_H_

#include <string>

#include "widgets.h"
#include "framed_widgets.h"
#include "forward_decs.h"
#include "faction.h"
#include "world.h"
#include "resource_type.h"
#include "texture.h"
#include "util.h"
#include "game_util.h"
#include "metrics.h"
#include "selection.h"
#include "tool_tips.h"

using std::string;

using Shared::Graphics::Texture2D;
using namespace Shared::Math;
using Shared::Util::replaceBy;
using Glest::Global::Metrics;

namespace Glest { namespace Gui_Mandate {

using namespace Widgets;
using namespace ProtoTypes;
using Entities::Faction;

class UserInterface;

WRAPPED_ENUM( TradeDisplaySection, TRADE )
//FACTION,

struct TradeDisplayButton {
	TradeDisplaySection	        m_section;
	int				            m_index;

	TradeDisplayButton(TradeDisplaySection s, int ndx) : m_section(s), m_index(ndx) {}

	TradeDisplayButton& operator=(const TradeDisplayButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const TradeDisplayButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const TradeDisplayButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class Width
// =====================================================

class Width {
public:
    int width;
};

// =====================================================
// 	class TradeBar
//
///	A bar that displays trade options
// =====================================================

class TradeBar : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
private:
	static const int cellWidthCount = 4;
	static const int cellHeightCount = 8;
	static const int tradeCellCount = cellWidthCount * cellHeightCount;
	static const int invalidIndex = -1;
    static const int cancelPos = cellWidthCount * cellHeightCount - 1;
	const World *world;
	Faction                    *m_faction;
	public:
	vector<TradeCommand>        m_tradeCommands;
	private:
	vector<string>              m_headerStrings;
	void setTradeCommand(int i, TradeCommand tc) {m_tradeCommands[i] = tc;}
	bool downLighted[tradeCellCount];
    bool m_selectingSecond;
    bool selectingPos;
    bool isSelectingPos() const {return selectingPos;}
    int activePos;
	int index[tradeCellCount];
	int m_selectedTradeIndex;

    int m_imageSize;

	void setSelectedTradePos(int i);
	TradeDisplayButton	m_hoverBtn,
					    m_pressedBtn;
	const FontMetrics *m_fontMetrics;
typedef vector<Width> Widths;
	bool  m_draggingWidget;
	Vec2i m_tradeOffset;
	int   m_updateCounter;
    struct SizeCollection {
		Vec2i tradeSize;
	};
    SizeCollection m_sizes;

	FuzzySize m_fuzzySize;
	TradeTip  *m_toolTip;
public:
	TradeBar(Container *parent, Vec2i pos);
	const Faction *getFaction() const {return m_faction;}
	void setToolTipText(const string &hdr, const string &tip, TradeDisplaySection i_section = TradeDisplaySection::TRADE);
    void onFirstTierSelect(int posTrade);
    void onSecondTierSelect(int posTrade);
	void resetState(bool redoDisplay = true);
	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	void setFuzzySize(FuzzySize fSize);
	void setSize();
	int getIndex(int i)								  {return index[i];}
    const TradeCommand *tradeCommands[tradeCellCount];
	const TradeCommand *getTradeCommand(int i)        {return tradeCommands[i];}
	void setUpImage(int i, const Texture2D *image) 	  {setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)  {setImage(image, i);}
	void setDownLighted(int i, bool lighted)		  {downLighted[i] = lighted;}
    void tradeButtonPressed(int posTrade);
	TradeTip* getTradeTip()                     {return m_toolTip;}
	void computeTradeInfo(int posTrade);
	void computeTradePanel();
	void computeTradeTip(const TradeCommand *tc, const ResourceType *rt);
	virtual void render() override;
	void layout();
	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_tradeOffset);}
	void persist();
	void reset();
	TradeDisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	TradeDisplayButton getHoverButton() const { return m_hoverBtn; }
	virtual string descType() const override { return "TradePanel"; }
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	void mouseOut() override;
	void invalidateActivePos() { activePos = -1; }
};

// =====================================================
// 	class TradeBarFrame
// =====================================================

class TradeBarFrame : public Frame {
private:
	TradeBar  *m_tradeBar;
	Button	  *m_tradeButton;
	CellStrip *m_tradePanel;

private:
	void doEnableShrinkExpand(int sz);

public:
	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	TradeBarFrame(Vec2i pos);
	TradeBar* getTradeBar() {return m_tradeBar;}

	void resetSize();
	virtual void render() override;
	void setPinned(bool v);
};

}}//end namespace

#endif
