// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GRAPHCOMPONENT_H_
#define _GLEST_GAME_GRAPHCOMPONENT_H_

#include <string>
#include <vector>

#include "math_util.h"
#include "font.h"
#include "input.h"

using std::string;
using std::vector;

using Shared::Graphics::Font;
using Shared::Platform::Key;

namespace Glest { namespace Graphics {

// ===========================================================
// 	class GraphicComponent
//
//	OpenGL renderer GUI components
// ===========================================================

class GraphicComponent {
public:
	static const float animSpeed;
	static const float fadeSpeed;

protected:
	int x, y, w, h;
	string text;
	const Font *font;
	bool enabled;
	bool visible;

	static float anim;
	static float fade;

public:
	GraphicComponent();
	virtual ~GraphicComponent(){}
	
	void init(int x, int y, int w, int h);
	
	int getX() const					{return x;}
	int getY() const					{return y;}
	int getW() const					{return w;}
	int getH() const					{return h;}
	bool getEnabled () const { return enabled; }
	void setEnabled ( bool enable ) { enabled = enable; }
	void show() { visible = true; }
	void hide() { visible = false; }
	bool isVisible() const { return visible; }
	const string &getText() const		{return text;}
	const Font *getFont() const		{return font;}
	bool isInBounds(int x, int y) const {
		return	   x >= this->x
				&& x < this->x + w
				&& y >= this->y
				&& y < this->y + h;
	}
	
	void setX(int x)					{this->x= x;}
	void setY(int y)					{this->y= y;}
	void setText(const string &text)	{this->text= text;}
	void setFont(const Font *font)	{this->font= font;}
	
	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);
	virtual void render() {}
	
	static void update();
	static void resetFade();
	static float getAnim()	{return anim;}
	static float getFade()	{return fade;}
};

// ===========================================================
// 	class GraphicProgressBar
/// Non-interactive component that displays progress using a 
/// changing image
// ===========================================================

class GraphicProgressBar: public GraphicComponent {
private:
	int progress;
	Font *font;

public:
	GraphicProgressBar();
	void init(int x, int y, int w, int h);

	void setProgress(int v) { progress = v; }
	int getProgress() { return progress; }
	virtual void render();
};

}}//end namespace

#endif
