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
class WidgetWindow : public Container, public MouseWidget, public KeyboardWidget, public WindowGl {
public:
	typedef WidgetWindow* Ptr;

private:
	typedef std::stack<Widget::Ptr>	WidgetStack;
	//typedef std::list<Layer::Ptr>	LayerList;
	typedef std::set<string>		NameSet;

protected:
	static WidgetWindow *instance;

private:
	Widget::Ptr	floatingWidget;
	KeyboardWidget::Ptr keyboardFocused;
	MouseWidget::Ptr mouseDownWidgets[MouseButton::COUNT];
	MouseWidget::Ptr lastMouseDownWidget;
	MouseButton lastMouseDownButton;

	WidgetList	toClean;
	WidgetList	updateList;
	WidgetStack mouseOverStack;

	TextRenderer *textRendererBM, *textRendererFT;

	float anim, slowAnim;
	Vec2i mousePos;
	bool modalFloater;

	Widget::Ptr findCommonAncestor(Widget::Ptr widget1, Widget::Ptr widget2);
	void unwindMouseOverStack(Widget::Ptr newTop);
	void unwindMouseOverStack();
	void doMouseInto(Widget::Ptr widget);
	void destroyFloater();
	void renderMouseCursor();

public:
	WidgetWindow();
	virtual ~WidgetWindow();

	static WidgetWindow* getInstance() { return instance; }

	void update();
	float getAnim() const { return anim; }
	virtual void clear();
	//void setFade(float v);

	void registerUpdate(Widget::Ptr widget);
	void unregisterUpdate(Widget::Ptr widget);

	void setFloatingWidget(Widget::Ptr floater, bool modal = false);
	void removeFloatingWidget(Widget::Ptr floater);

	void aquireKeyboardFocus(KeyboardWidget::Ptr widget);
	void releaseKeyboardFocus(KeyboardWidget::Ptr widget);

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
	/*
	virtual bool mouseDown(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseUp(MouseButton btn, Vec2i pos)				{ return false; }
	virtual bool mouseMove(Vec2i pos)							{ return false; }
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos)	{ return false; }
	virtual bool mouseWheel(Vec2i pos, int z)					{ return false; }
	virtual bool keyDown(Key key)								{ return false; }
	virtual bool keyUp(Key key)									{ return false; }
	virtual bool keyPress(char c)								{ return false; }
	*/
	virtual Vec2i getPrefSize() const	{ return Vec2i(-1);			}
	virtual Vec2i getMinSize() const	{ return Vec2i(800, 600);	}
	virtual Vec2i getMaxSize() const	{ return Vec2i(-1);			}

	TextRenderer* getTextRenderer(bool ft) {
		return ft ? textRendererFT : textRendererBM;
	}

	//virtual Widget::Ptr getWidgetAt(const Vec2i &pos);// { return layers.front()->getWidgetAt(pos); }
	//virtual void addChild(Widget::Ptr child) { layers.front()->addChild(child); child->setParent(layers.front()); }
	//virtual void remChild(Widget::Ptr child) { layers.front()->remChild(child); child->setParent(layers.front()); }

	virtual void render();

	virtual string desc() { return string("[Window: ") + descPosDim() + "]"; }
};

}}

#endif
