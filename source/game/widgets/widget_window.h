// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_WINDOW_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_WINDOW_INCLUDED_

#include <stack>
#include <list>

#include "window_gl.h"
#include "sigslot.h"
#include "timer.h"

#include "widgets_base.h"

using Shared::Platform::WindowGl;

namespace Glest { namespace Widgets {

// =====================================================
// class WidgetWindow
// =====================================================
/** top level widget container, derives from Shared::Platform::WindowGl & Glest::Widgets::Container,
  * and is base class to Glest::Main::Program */
class WidgetWindow : public Container, public MouseWidget, public KeyboardWidget, public WindowGl {
private:
	typedef std::stack<Widget*> WidgetStack;
	typedef std::set<Widget*>   WidgetSet;
	typedef std::set<string>    NameSet;
	typedef std::stack<Rect2i>  ClipStack;

protected:
	static WidgetWindow *instance;

private:
	WidgetConfig *m_config;
	Widget*	floatingWidget;
	KeyboardWidget* keyboardFocused;
	KeyboardWidget* lastKeyDownWidget;
	MouseWidget* mouseDownWidgets[MouseButton::COUNT];
	MouseWidget* lastMouseDownWidget;
	MouseButton lastMouseDownButton;
	ClipStack   m_clipStack;

	WidgetList	toClean;
	WidgetList	updateList;
	WidgetList  addUpdateQueue;
	WidgetList  remUpdateQueue;

	WidgetStack mouseOverStack;

	TextRenderer *textRendererFT;

	float anim, slowAnim;
	Vec2i mousePos;
	bool modalFloater;
	bool awaitingMouseUp;
	//const Texture2D *mouseIcon;

	MouseCursor *m_mouseCursor;
	//Imageset *mouseMain;
	//Animset *mouseAnimations;

private:
	void setDisplaySettings();
	void restoreDisplaySettings();

	Widget* findCommonAncestor(Widget* widget1, Widget* widget2);
	Widget* getWidgetForMouseEvent();
	void unwindMouseOverStack(Widget* newTop);
	void unwindMouseOverStack();
	void doMouseInto(Widget *widget);
	void destroyFloater();
	void setScissor(const Rect2i &rect);

public:
	WidgetWindow();
	virtual ~WidgetWindow();

	static WidgetWindow* getInstance() { return instance; }
	WidgetConfig* getConfig() { return m_config; }

	void update();
	float getAnim() const { return anim; }
	float getSlowAnim() const { return slowAnim; }
	virtual void clear() override;

	void pushClipRect(const Vec2i &pos, const Vec2i &size);
	void popClipRect();

	void registerUpdate(Widget* widget);
	void unregisterUpdate(Widget* widget);

	void setFloatingWidget(Widget* floater, bool modal = false);
	void removeFloatingWidget(Widget* floater);

	void aquireKeyboardFocus(KeyboardWidget* widget);
	void releaseKeyboardFocus(KeyboardWidget* widget);

	MouseCursor& getMouseCursor() { return *m_mouseCursor; }

	virtual void resize(VideoMode mode) override;

protected: // Shared::Platform::Window virtual events
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton) override;
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton) override;
	virtual void eventMouseMove(int x, int y, const MouseState &mouseState) override;
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton) override;
	virtual void eventMouseWheel(int x, int y, int zDelta) override;
	virtual void eventKeyDown(const Key &key) override;
	virtual void eventKeyUp(const Key &key) override;
	virtual void eventKeyPress(char c) override;

public: // MouseWidget & TextWidget virtual events
	/*
	virtual bool mouseDown(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseUp(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool mouseMove(Vec2i pos)							{ return false; }
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos)	{ return false; }
	virtual bool mouseWheel(Vec2i pos, int z)					{ return false; }
	virtual bool keyDown(Key key)								{ return false; }
	virtual bool keyUp(Key key)									{ return false; }
	virtual bool keyPress(char c)								{ return false; }
	*/
	virtual Vec2i getPrefSize() const override { return Vec2i(-1);			}
	virtual Vec2i getMinSize() const override { return Vec2i(800, 600);	}
	virtual Vec2i getMaxSize() const override { return Vec2i(-1);			}

	TextRenderer* getTextRenderer() {
		return textRendererFT;
	}

	virtual void render() override;

	virtual string descType() const override { return "Window"; }
};

#define g_widgetWindow (*WidgetWindow::getInstance())

}}

#endif
