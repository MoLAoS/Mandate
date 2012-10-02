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

#ifndef _GLEST_GAME_FACTIONDISPLAY_H_
#define _GLEST_GAME_FACTIONDISPLAY_H_

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
#include "faction_command.h"

using std::string;

using Shared::Graphics::Texture2D;
using namespace Shared::Math;
using Shared::Util::replaceBy;
using Glest::Global::Metrics;

namespace Glest { namespace Gui_Mandate {

using namespace Widgets;
using namespace ProtoTypes;
using namespace Gui;
using Entities::Faction;

WRAPPED_ENUM( FactionDisplaySection, COMMANDS )

struct FactionDisplayButton {
	FactionDisplaySection	        m_section;
	int				        m_index;

	FactionDisplayButton(FactionDisplaySection s, int ndx) : m_section(s), m_index(ndx) {}

	FactionDisplayButton& operator=(const FactionDisplayButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const FactionDisplayButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const FactionDisplayButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class FactionDisplay
//
///	Display for unit commands, and unit selection
// =====================================================

class FactionDisplay : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 4;

	static const int commandCellCount = cellWidthCount * cellHeightCount;

	static const int invalidIndex = -1;

private:
    const Faction*            m_faction;
    vector<const UnitType*> m_unitTypes;
    vector<FactionBuild> m_factionBuilds;

    int   m_iconSize;
	bool  m_draggingWidget;

	UserInterface *m_ui;
	bool downLighted[commandCellCount];
	int index[commandCellCount];
	const CommandType *commandTypes[commandCellCount];
	CmdClass commandClasses[commandCellCount];
	int m_selectedCommandIndex;
	int m_logo;

	int m_imageSize;

	struct SizeCollection {
		Vec2i logoSize;
		Vec2i commandSize;
	};
	SizeCollection m_sizes;

	FuzzySize m_fuzzySize;

	Vec2i	m_commandOffset;

	FactionDisplayButton	m_hoverBtn,
					        m_pressedBtn;

	CommandTip  *m_toolTip;
    TradeTip  *m_buildTip;
	const FontMetrics *m_fontMetrics;

private:
	void layout();
	void renderProgressBar();

public:
	FactionDisplay(Container *parent, UserInterface *ui, Vec2i pos);

    bool building;
    FactionBuild currentFactionBuild;

	//get
	const Faction *getFaction() const               {return m_faction;}
	int getBuildingCount() const                    {return m_unitTypes.size();}
	const UnitType *getBuilding(int i) const        {return m_unitTypes[i];}
	int getBuildCount() const                       {return m_factionBuilds.size();}
	FactionBuild getFactionBuild(int i) const       {return m_factionBuilds[i];}
	TradeTip *getBuildTip() const                    {return m_buildTip;}
	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	string getPortraitTitle() const					{return TextWidget::getText(0);}
	string getPortraitText() const					{return TextWidget::getText(1);}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	const CommandType *getCommandType(int i)        {return commandTypes[i];}
	CmdClass getCommandClass(int i)                 {return commandClasses[i];}
	int getSelectedCommandIndex() const             {return m_selectedCommandIndex;}
	CommandTip* getCommandTip()                     {return m_toolTip;}

	//set
	void setSize();
	void setFuzzySize(FuzzySize fSize);
	void setPortraitTitle(const string title);
	void setPortraitText(const string &text);
	void setToolTipText2(const string &hdr, const string &tip, FactionDisplaySection i_section = FactionDisplaySection::COMMANDS);
	void addToolTipReq(const DisplayableType *dt, bool ok, const string &txt);

	void setUpImage(int i, const Texture2D *image) 		 {setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	 {setImage(image, i);}
	void setCommandType(int i, const CommandType *ct)	 {commandTypes[i]= ct;}
	void setCommandClass(int i, const CmdClass cc)	     {commandClasses[i]= cc;}
	void setDownLighted(int i, bool lighted)			 {downLighted[i]= lighted;}
	void setIndex(int i, int value)						 {index[i] = value;}
	void setSelectedCommandPos(int i);

	//misc
	void init(const Faction *faction, std::set<const UnitType*> &types);

	void computeBuildTip(FactionBuild fb);
	void computeBuildInfo(int posDisplay);
	void onFirstTierSelect(int posBuild);
    void buildButtonPressed(int posDisplay);

	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_commandOffset);}
	FactionDisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	FactionDisplayButton getHoverButton() const { return m_hoverBtn; }
	void persist();
	void reset();

	virtual void render() override;
	virtual string descType() const override { return "FactionPanel"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual void mouseOut() override;

	public:
    void computeFactionCommandPanel();
    void computeBuildPanel();


};

// =====================================================
//  class FactionDisplayFrame
// =====================================================

class FactionDisplayFrame : public Frame {
private:
	FactionDisplay *m_factionDisplay;
	UserInterface *m_ui;

public:
	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	FactionDisplayFrame(UserInterface *ui, Vec2i pos);
	FactionDisplay* getFactionDisplay() {return m_factionDisplay;}

	void resetSize();

	virtual void render() override;
	void setPinned(bool v);



};

}}//end namespace

#endif
