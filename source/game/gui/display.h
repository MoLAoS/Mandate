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
	//static const int cellSideCount = 4;
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 4;


	static const int selectionCellCount = cellWidthCount * cellHeightCount;
	static const int commandCellCount = cellWidthCount * cellHeightCount;
	static const int transportCellCount = cellWidthCount * cellHeightCount / 2;

	static const int colorCount = 4;
	static const int imageSize = 32;
	static const int invalidPos = -1;

private:
	UserInterface *m_ui;
	bool downLighted[commandCellCount];
	int index[commandCellCount];
	const CommandType *commandTypes[commandCellCount];
	CommandClass commandClasses[commandCellCount];
	Vec3f colors[colorCount];
	int m_progress;
	int currentColor;
	int m_selectedCommandIndex;//downSelectedPos;
	int m_logo;
	int m_autoRepairOn, m_autoRepairOff, m_autoRepairMixed;
	int m_autoAttackOn, m_autoAttackOff, m_autoAttackMixed;
	int m_autoFleeOn, m_autoFleeOff, m_autoFleeMixed;

	// some stuff that should be in a superclass ... (Widgets::Frame ?)
	bool m_draggingWidget;
	Vec2i m_moveOffset;

	Vec2i	m_upImageOffset,	// x,y offset for selected unit portrait(s)
			m_downImageOffset,	// x,y offset for command buttons
			m_carryImageOffset, // x,y offset for loaded unit portrait(s)
			m_progressPos;		// x,y offset for progress bar
	int m_progPrecentPos;		// progress bar percentage (and -1 when no progress bar)
	Font *m_font;

	DisplayButton	m_hoverBtn,		// section/index of button mouse is over
					m_pressedBtn;	// section/index of button that received a mouse down event

	ToolTip	*m_toolTip;

private:
	void renderProgressBar();
	int getImageOverlayIndex(AutoCmdFlag f, AutoCmdState s);
	bool isInBounds(const Vec2i &pos, Vec2i &out_pos = Vec2i());

public:
	Display(UserInterface *ui, Vec2i pos);

	//get
	string getPortraitTitle() const					{return TextWidget::getText(0);}
	string getPortraitText() const					{return TextWidget::getText(1);}
	string getOrderQueueText() const				{return TextWidget::getText(2);}
	string getTransportedLabel() const				{return TextWidget::getText(4);}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	const CommandType *getCommandType(int i)		{return commandTypes[i];}
	CommandClass getCommandClass(int i)				{return commandClasses[i];}
	Vec3f getColor() const							{return colors[currentColor];}
	int getProgressBar() const						{return m_progress;}
	int getSelectedCommandIndex() const				{return m_selectedCommandIndex;}

	//set
	void setSize();
	void setPortraitTitle(const string title);
	void setPortraitText(const string &text);
	void setOrderQueueText(const string &text);
	void setToolTipText(const string &i_txt, DisplaySection i_section = DisplaySection::COMMANDS);
	void setTransportedLabel(bool v);

	void setUpImage(int i, const Texture2D *image) 		{setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	{setImage(image, selectionCellCount + i);}
	void setCarryImage(int i, const Texture2D *image)	{setImage(image, selectionCellCount + commandCellCount + i);}
	void setCommandType(int i, const CommandType *ct)	{commandTypes[i]= ct;}
	void setCommandClass(int i, const CommandClass cc)	{commandClasses[i]= cc;}
	void setDownLighted(int i, bool lighted)			{downLighted[i]= lighted;}
	void setIndex(int i, int value)						{index[i] = value;}
	void setProgressBar(int i);
	void setLoadInfo(const string &txt);
	void setDownSelectedPos(int i);

	//misc
	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_downImageOffset);}
	DisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	DisplayButton getHoverButton() const { return m_hoverBtn; }

	void switchColor() {currentColor = (currentColor + 1) % colorCount;}

	virtual void render();
	virtual string desc() { return string("[DisplayPanel: ") + descPosDim() + "]"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos);
	virtual void mouseOut();
};

}}//end namespace

#endif
