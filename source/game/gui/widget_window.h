// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_WINDOW_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_WINDOW_INCLUDED_

#include <stack>
#include <list>

#include "window_gl.h"
#include "sigslot.h"

#include "widgets_base.h"

using Shared::Platform::WindowGl;

namespace Glest { namespace Widgets {

// =====================================================
// class WidgetWindow
// =====================================================
/** top level container */
class WidgetWindow : public Container, public WindowGl {
private:
	WidgetPtr	keyboardFocused,
				floatingWidget,
				mouseDownWidgets[MouseButton::COUNT];
	std::stack<WidgetPtr> mouseOverStack;
	std::list<LayerPtr> layers;
	std::set<string> layerNames;
	int layerIdCounter;
	LayerPtr rootLayer;
	WidgetList toClean;
	TextRenderer *textRendererBM, *textRendererFT;

	float anim;
	void unwindMouseOverStack();
	void doMouseInto(WidgetPtr widget);
	void changeActiveLayer(list<LayerPtr>::iterator &it);
	Vec2i mousePos;

public:
	WidgetWindow();
	virtual ~WidgetWindow();
	void update();
	float getAnim() const { return anim; }

	virtual void clear();

	void setActiveLayer(const string &layer);
	void setActiveLayer(const int layer);
	LayerPtr addNewLayer(const string &name, bool activate = true);
	LayerPtr getLayer(const string &layer);
	LayerPtr getLayer(const int layer);

	void setFloatingWidget(WidgetPtr floater);
	void removeFloatingWidget(WidgetPtr floater);
	
	void aquireKeyboardFocus(WidgetPtr widget);
	void releaseKeyboardFocus(WidgetPtr widget);

protected: // Shared::Platform::Window virtual events
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton);
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton);
	virtual void eventMouseMove(int x, int y, const MouseState &mouseState);
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton);
	virtual void eventMouseWheel(int x, int y, int zDelta);
	virtual void eventKeyDown(const Key &key);
	virtual void eventKeyUp(const Key &key);
	virtual void eventKeyPress(char c);

public: // Glest::Widgets::Widget virtual events
	virtual bool mouseDown(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseUp(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseMove(Vec2i pos)							{ return false; }
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos)	{ return false; }
	virtual bool mouseWheel(Vec2i pos, int z)					{ return false; }
	virtual bool keyDown(Key key)								{ return false; }
	virtual bool keyUp(Key key)									{ return false; }
	virtual bool keyPress(char c)								{ return false; }

	TextRenderer* getTextRenderer(bool ft) {
		return ft ? textRendererFT : textRendererBM;
	}

	virtual WidgetPtr getWidgetAt(const Vec2i &pos) { return layers.front()->getWidgetAt(pos); }
	virtual void addChild(WidgetPtr child) { layers.front()->addChild(child); child->setParent(layers.front()); }
	virtual void remChild(WidgetPtr child) { layers.front()->remChild(child); child->setParent(layers.front()); }

	virtual void render();

	virtual string desc() { return string("[Window: ") + descPosDim() + "]"; }
};

}}

#endif
