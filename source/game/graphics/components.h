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

#ifndef _GLEST_GAME_GRAPHCOMPONENT_H_
#define _GLEST_GAME_GRAPHCOMPONENT_H_

#include <string>
#include <vector>

#include "font.h"
#include "input.h"

using std::string;
using std::vector;

using Shared::Graphics::Font2D;
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
	const Font2D *font;
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
	bool isVisible() { return visible; }
	const string &getText() const		{return text;}
	const Font2D *getFont() const		{return font;}
	bool isInBounds(int x, int y) const {
		return	   x >= this->x
				&& x < this->x + w
				&& y >= this->y
				&& y < this->y + h;
	}
	
	void setX(int x)					{this->x= x;}
	void setY(int y)					{this->y= y;}
	void setText(const string &text)	{this->text= text;}
	void setFont(const Font2D *font)	{this->font= font;}
	
	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);
	virtual void render() {} // TODO: should this have an implementation ? YES!
	
	static void update();
	static void resetFade();
	static float getAnim()	{return anim;}
	static float getFade()	{return fade;}
};

// ===========================================================
// 	class GraphicPanel

/// GraphicPanel is a container for other GraphicComponents. 
/// Common actions are performed on the contained components 
/// with a single call to the related panel methods.
// ===========================================================

class GraphicPanel : public GraphicComponent {
public:
	
private:
	vector<GraphicComponent*> components;

public:
	GraphicPanel() : GraphicComponent() {}

	void init(int x, int y, int w, int h);
	/** Add a GraphicComponent to the panel. Pointer should be managed by caller. */
	void addComponent(GraphicComponent *gc);
	
	// common actions
	/** Only movement within the panel area is passed to contained components. */
	virtual bool mouseMove(int x, int y);
	void update();
	virtual void render();
};

// ===========================================================
// 	class GraphicLabel
// ===========================================================

class GraphicLabel: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	bool centered;

public:
	GraphicLabel() : GraphicComponent(), centered(false) {}
	void init(int x, int y, int w=defW, int h=defH, bool centered= false);

	bool getCentered() const	{return centered;}

	void setCentered(bool centered)	{this->centered= centered;}
	virtual void render();
};

// ===========================================================
// 	class GraphicButton
// ===========================================================

class GraphicButton: public GraphicComponent{
public:
	static const int defH;
	static const int defW;

private:
	bool lighted;

public:
	GraphicButton() : GraphicComponent(), lighted(false) {}
	void init(int x, int y, int w=defW, int h=defH);

	bool getLighted() const			{return lighted;}

	void setLighted(bool lighted)	{this->lighted= lighted;}
	virtual bool mouseMove(int x, int y);
	virtual void render();
};

// ===========================================================
// 	class GraphicListBox
// ===========================================================

class GraphicListBox: public GraphicComponent{
public:
	static const int defH;
	static const int defW;

private:
	GraphicButton graphButton1, graphButton2;
	vector<string> items;
	int selectedItemIndex;

public:
	GraphicListBox();
	void init(int x, int y, int w=defW, int h=defH);
	
	int getItemCount() const				{return items.size();}
	int getSelectedItemIndex() const		{return selectedItemIndex;}
	string getSelectedItem() const			{return items[selectedItemIndex];}
	const GraphicButton *getButton1() const	{return &graphButton1;}
	const GraphicButton *getButton2() const	{return &graphButton2;}
	
	void pushBackItem(string item);
	void setItems(const vector<string> &items);
	void setSelectedItemIndex(int index);
	void setSelectedItem(string item);
	
	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);
	virtual void render();
};

// ===========================================================
// 	class GraphicMessageBox
// ===========================================================

class GraphicMessageBox: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	GraphicButton button1;
	GraphicButton button2;
	int buttonCount;
   string header;

public:
	GraphicMessageBox();
	void init(const string &text, const string &button1Str, const string &button2Str = "");
	//void init(const string &text, const string &button1Str); //redundant
	
	int getButtonCount() const				{return buttonCount;}
	const GraphicButton *getButton1() const	{return &button1;}
	const GraphicButton *getButton2() const	{return &button2;}
   string getHeader () const { return header; }
   void setHeader ( string text ) { header = text; }
	
	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);
	bool mouseClick(int x, int y, int &clickedButton);
	virtual void render();
	
	void setText(const string &text) {
		GraphicComponent::setText(text);
		layout();
	}

private:
	//void init(); //redundant
	void layout();
};

// ===========================================================
// 	class GraphicTextEntry
// ===========================================================

class GraphicTextEntry: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	bool activated1, activated2;

public:
	GraphicTextEntry() : GraphicComponent(), activated1(false), activated2(false) {}
	void init(int x, int y, int w=defW, int h=defH);

	bool isActivated() const	{ return activated1 || activated2; }

	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);
	void keyPress(char c);
	void keyDown(const Key &key);
	void setFocus()				{activated1 = true;}
	virtual void render();
};

// ===========================================================
// 	class GraphicTextEntryBox
// ===========================================================

class GraphicTextEntryBox: public GraphicComponent {
public:
	static const int defH;
	static const int defW;

private:
	GraphicButton button1;
	GraphicButton button2;
	GraphicTextEntry entry;
	GraphicLabel label;

public:
	GraphicTextEntryBox();
	void init(const string &button1Str, const string &button2Str, const string &title="", const string &text="", const string &entryText="");

	const GraphicButton *getButton1() const	{return &button1;}
	const GraphicButton *getButton2() const	{return &button2;}
	const GraphicTextEntry *getEntry() const{return &entry;}
	const GraphicLabel *getLabel() const	{return &label;}

	virtual bool mouseMove(int x, int y);
	virtual bool mouseClick(int x, int y);
	bool mouseClick(int x, int y, int &clickedButton);
	void keyPress(char c)					{entry.keyPress(c);}
	void keyDown(const Key &key);
	void setFocus()							{entry.setFocus();}
	virtual void render();

};

// ===========================================================
// 	class GraphicProgressBar
/// Non-interactive component that displays progress using a 
/// changing image
// ===========================================================

class GraphicProgressBar: public GraphicComponent {
private:
	int progress;
	Font2D *font;

public:
	GraphicProgressBar();
	void init(int x, int y, int w, int h);

	void setProgress(int v) { progress = v; }
	int getProgress() { return progress; }
	virtual void render();
};

}}//end namespace

#endif
