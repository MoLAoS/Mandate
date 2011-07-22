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

#ifndef _GLEST_GAME_DISPLAY_H_
#define _GLEST_GAME_DISPLAY_H_

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

WRAPPED_ENUM( DisplaySection, SELECTION, COMMANDS, TRANSPORTED )

struct DisplayButton {
	DisplaySection	m_section;
	int				m_index;

	DisplayButton(DisplaySection s, int ndx) : m_section(s), m_index(ndx) {}

	DisplayButton& operator=(const DisplayButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const DisplayButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const DisplayButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class Display
//
///	Display for unit commands, and unit selection
// =====================================================

class Display : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	// portrait and command sub-panel width and height, in 'cells'
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 4;
	// transported sub-panel is cellWidthCount * (cellHeightCount / 2)

	static const int selectionCellCount = cellWidthCount * cellHeightCount;
	static const int commandCellCount = cellWidthCount * cellHeightCount;
	static const int transportCellCount = cellWidthCount * cellHeightCount / 2;

	static const int invalidPos = -1;

private:
	UserInterface *m_ui; // 'conceptual' owner, but not a widget, so not our 'parent'
	bool downLighted[commandCellCount]; // flag per command cell, false to draw 'disabled' (darkened)
	int index[commandCellCount];        // index per command cell, identifies producibles in two tier selections
	const CommandType *commandTypes[commandCellCount]; // CommandType per command cell, only valid if selection.isUniform()
	CmdClass commandClasses[commandCellCount];         // CommandClass per command cell, use when !selection.isUniform()
	int m_progress;    // 0-100 to show progress bar at that percentage, -1 == no progress bar.
	int m_selectedCommandIndex; // index of command (in commnadTypes and/or commandClasses) used to identify original command in two-tier selects
	int m_logo;                 // index of logo image (in ImageWidget::textures)
	// image indices (in ImageWidget::textures) for auto command flag icons
	int m_autoRepairOn, m_autoRepairOff, m_autoRepairMixed;
	int m_autoAttackOn, m_autoAttackOff, m_autoAttackMixed;
	int m_autoFleeOn, m_autoFleeOff, m_autoFleeMixed;

	int m_imageSize;

	struct SizeCollection {
		Vec2i portraitSize;
		Vec2i logoSize;
		Vec2i commandSize;
		Vec2i transportSize;
	};
	SizeCollection m_sizes;//[3];

	FuzzySize m_fuzzySize;

	Vec2i	m_portraitOffset,	// x,y offset for selected unit portrait(s)
			m_commandOffset,	// x,y offset for command buttons
			m_carryImageOffset, // x,y offset for loaded unit portrait(s)
			m_progressPos;		// x,y offset for progress bar
	int m_progPrecentPos;		// x offset to draw progress bar percentage text (and -1 when no progress bar)

	DisplayButton	m_hoverBtn,		// section/index of button mouse is over
					m_pressedBtn;	// section/index of button that received a mouse down event

	CommandTip  *m_toolTip;

private:
	void renderProgressBar();
	int getImageOverlayIndex(AutoCmdFlag f, AutoCmdState s);

public:
	Display(Container *parent, UserInterface *ui, Vec2i pos);

	//get
	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	string getPortraitTitle() const					{return TextWidget::getText(0);}
	string getPortraitText() const					{return TextWidget::getText(1);}
	string getOrderQueueText() const				{return TextWidget::getText(2);}
	string getTransportedLabel() const				{return TextWidget::getText(4);}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	const CommandType *getCommandType(int i)        {return commandTypes[i];}
	CmdClass getCommandClass(int i)                 {return commandClasses[i];}
	int getProgressBar() const                      {return m_progress;}
	int getSelectedCommandIndex() const             {return m_selectedCommandIndex;}
	CommandTip* getCommandTip()                     {return m_toolTip;}

	//set
	void setSize();
	void setFuzzySize(FuzzySize fSize);
	void setPortraitTitle(const string title);
	void setPortraitText(const string &text);
	void setOrderQueueText(const string &text);
	void setToolTipText2(const string &hdr, const string &tip, DisplaySection i_section = DisplaySection::COMMANDS);
	void addToolTipReq(const DisplayableType *dt, bool ok, const string &txt);
	//void setToolTipText(const string &i_txt, DisplaySection i_section = DisplaySection::COMMANDS);
	void setTransportedLabel(bool v);

	void setUpImage(int i, const Texture2D *image) 		{setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	{setImage(image, selectionCellCount + i);}
	void setCarryImage(int i, const Texture2D *image)	{setImage(image, selectionCellCount + commandCellCount + i);}
	void setCommandType(int i, const CommandType *ct)	{commandTypes[i]= ct;}
	void setCommandClass(int i, const CmdClass cc)	{commandClasses[i]= cc;}
	void setDownLighted(int i, bool lighted)			{downLighted[i]= lighted;}
	void setIndex(int i, int value)						{index[i] = value;}
	void setProgressBar(int i);
	void setLoadInfo(const string &txt);
	void setSelectedCommandPos(int i);

	//misc
	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_commandOffset);}
	DisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	DisplayButton getHoverButton() const { return m_hoverBtn; }

	virtual void render() override;
	virtual string descType() const override { return "DisplayPanel"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual void mouseOut() override;
};

// =====================================================
//  class DisplayFrame
// =====================================================

class DisplayFrame : public Frame {
private:
	Display *m_display;
	UserInterface *m_ui;

	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	DisplayFrame(UserInterface *ui, Vec2i pos);
	Display* getDisplay() {return m_display;}

	void resetSize();

	virtual void render() override;
	void setPinned(bool v);
};


}}//end namespace

#endif
