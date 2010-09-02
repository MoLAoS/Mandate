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

// =====================================================
// 	class Display
//
///	Display for unit commands, and unit selection
// =====================================================

class Display : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	static const int cellSideCount = 4;
	static const int upCellCount = cellSideCount * cellSideCount;
	static const int downCellCount = cellSideCount * cellSideCount;
	static const int carryCellCount = cellSideCount * cellSideCount;
	static const int colorCount = 4;
	static const int imageSize = 32;
	static const int invalidPos = -1;
	static const int downY = imageSize * 9;
	static const int carryY = imageSize * 2;
	static const int infoStringY = imageSize * 4;

private:
	UserInterface *m_ui;
	bool downLighted[downCellCount];
	int index[downCellCount];
	const CommandType *commandTypes[downCellCount];
	CommandClass commandClasses[downCellCount];
	Vec3f colors[colorCount];
	int m_progress;
	int currentColor;
	int downSelectedPos;

	// some stuff that should be in a superclass ... (Widgets::Frame ?)
	bool m_draggingWidget;
	Vec2i m_moveOffset;

	Vec2i	m_upImageOffset,	// x,y offset for selected unit portrait(s)
			m_downImageOffset,	// x,y offset for command buttons
			m_carryImageOffset, // x,y offset for loaded unit portrait(s)
			m_progressPos;		// x,y offset for progress bar
	int m_progPrecentPos;		// progress bar percentage (and -1 when no progress bar)
	Font *m_font;
	int m_mouseDownNdx;			// index of command button that received a mouse down event

private:
	void renderProgressBar();

public:
	Display(UserInterface *ui, Vec2i pos, Vec2i size);

	//get
	string getTitle() const							{return TextWidget::getText(0);}
	string getText() const							{return TextWidget::getText(1);}
	string getInfoText() const						{return TextWidget::getText(2);}
	string getTransportedLabel() const				{return TextWidget::getText(4);}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	const CommandType *getCommandType(int i)		{return commandTypes[i];}
	CommandClass getCommandClass(int i)				{return commandClasses[i];}
	Vec3f getColor() const							{return colors[currentColor];}
	int getProgressBar() const						{return m_progress;}
	int getDownSelectedPos() const					{return downSelectedPos;}

	//set
	void setSize();
	void setTitle(const string title);
	void setText(const string &text);
	void setInfoText(const string &infoText);
	void setTransportedLabel(bool v);

	void setUpImage(int i, const Texture2D *image) 		{setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	{setImage(image, upCellCount + i);}
	void setCarryImage(int i, const Texture2D *image)	{setImage(image, upCellCount + downCellCount + i);}
	void setCommandType(int i, const CommandType *ct)	{commandTypes[i]= ct;}
	void setCommandClass(int i, const CommandClass cc)	{commandClasses[i]= cc;}
	void setDownLighted(int i, bool lighted)			{downLighted[i]= lighted;}
	void setIndex(int i, int value)						{index[i] = value;}
	void setProgressBar(int i);
	void setDownSelectedPos(int i);

	//misc
	void clear();

	int computeIndex(Vec2i imgOffset, Vec2i pos);
	int computeDownIndex(int x, int y) { return computeIndex(m_downImageOffset, Vec2i(x,y) - getPos()); }

	void switchColor() {currentColor = (currentColor + 1) % colorCount;}

	virtual void render();
	virtual string desc() { return string("[DisplayPanel: ") + descPosDim() + "]"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos);
	virtual void mouseOut();
};

class ResourceBar : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
private:
	const Faction *m_faction;
	vector<const ResourceType*> m_resourceTypes;
	vector<string> m_headerStrings;
	bool m_draggingWidget;
	Vec2i m_moveOffset;

public:
	ResourceBar(const Faction *faction, std::set<const ResourceType*> &types);
	~ResourceBar();

	virtual void update();
	virtual void render();
	virtual string desc() { return string("[ResourceBar: ") + descPosDim() + "]"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
};

}}//end namespace

#endif
