// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
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
#include "command_type.h"
#include "game_util.h"
//#include "network_status.h"
#include "metrics.h"

using std::string;

using Shared::Graphics::Texture2D;
using Shared::Math::Vec3f;
using Shared::Util::replaceBy;
using Glest::Global::Metrics;

namespace Glest { namespace Gui {

// =====================================================
// 	class Display
//
///	Display for unit commands, and unit selection
// =====================================================

class Display {
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
	string title;
	string text;
	string infoText;
	const Texture2D *upImages[upCellCount];
	const Texture2D *downImages[downCellCount];
	const Texture2D *carryImages[carryCellCount];
	bool downLighted[downCellCount];
	int index[downCellCount];
	const CommandType *commandTypes[downCellCount];
	CommandClass commandClasses[downCellCount];
	Vec3f colors[colorCount];
	int progressBar;
	int currentColor;
	int downSelectedPos;

public:
	Display();

	//get
	string getTitle() const							{return title;}
	string getText() const							{return text;}
	string getInfoText() const						{return infoText;}
	const Texture2D *getUpImage(int index) const	{return upImages[index];}
	const Texture2D *getDownImage(int index) const	{return downImages[index];}
	const Texture2D *getCarryImage(int index) const	{return carryImages[index];}
	int getIndex(int i)								{return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}
	const CommandType *getCommandType(int i)		{return commandTypes[i];}
	CommandClass getCommandClass(int i)				{return commandClasses[i];}
	Vec3f getColor() const							{return colors[currentColor];}
	int getProgressBar() const						{return progressBar;}
	int getDownSelectedPos() const					{return downSelectedPos;}

	//set
	void setTitle(const string title)					{this->title= Util::formatString(title);}
	void setText(const string &text)					{this->text= Util::formatString(text);}
	void setInfoText(const string infoText)				{this->infoText= Util::formatString(infoText);}
	void setUpImage(int i, const Texture2D *image) 		{upImages[i]= image;}
	void setDownImage(int i, const Texture2D *image)	{downImages[i]= image;}
	void setCarryImage(int i, const Texture2D *image)	{carryImages[i]= image;}
	void setCommandType(int i, const CommandType *ct)	{commandTypes[i]= ct;}
	void setCommandClass(int i, const CommandClass cc)	{commandClasses[i]= cc;}
	void setDownLighted(int i, bool lighted)			{downLighted[i]= lighted;}
	void setIndex(int i, int value)						{index[i] = value;}
	void setProgressBar(int i)							{progressBar= i;}
	void setDownSelectedPos(int i)						{downSelectedPos= i;}

	//misc
	void clear();
	int computeDownIndex(int x, int y);
	int computeCarryIndex(int x, int y);
	void switchColor() {currentColor = (currentColor + 1) % colorCount;}

	int computeCarryX(int index) const {
		return (index % cellSideCount) * imageSize;
	}
	
	int computeCarryY(int index) const {
		return Display::carryY - (index / cellSideCount)*imageSize - imageSize;
	}

	int computeDownX(int index) const {
		return (index % cellSideCount) * imageSize;
	}
	
	int computeDownY(int index) const {
		return Display::downY - (index / cellSideCount)*imageSize - imageSize;
	}
	
	int computeUpX(int index) const {
		return (index % cellSideCount) * imageSize;
	}
	
	int computeUpY(int index) const {
		return Metrics::getInstance().getDisplayH() - (index / cellSideCount) * imageSize - imageSize;
	}
};

}}//end namespace

#endif
