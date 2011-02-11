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

namespace Glest { 
	namespace Graphics { class Imageset; }
	namespace Widgets {
class Animset;

WRAPPED_ENUM(MouseAppearance, DEFAULT, ICON, CAMERA_MOVE)

// =====================================================
// class WidgetWindow
// =====================================================
/** top level container */
class WidgetWindow : public Container, public MouseWidget, public KeyboardWidget, public WindowGl {
private:
	typedef std::stack<Widget*>	WidgetStack;
	//typedef std::list<Layer*>	LayerList;
	typedef std::set<string>		NameSet;

protected:
	static WidgetWindow *instance;

private:
	Widget*	floatingWidget;
	KeyboardWidget* keyboardFocused;
	KeyboardWidget* lastKeyDownWidget;
	MouseWidget* mouseDownWidgets[MouseButton::COUNT];
	MouseWidget* lastMouseDownWidget;
	MouseButton lastMouseDownButton;

	WidgetList	toClean;
	WidgetList	updateList;
	WidgetStack mouseOverStack;

	TextRenderer *textRendererFT;

	float anim, slowAnim;
	Vec2i mousePos;
	bool modalFloater;
	const Texture2D *mouseIcon;

	Texture2D *m_mouseTexture;
	Glest::Graphics::Imageset *mouseMain;
	Animset *mouseAnimations;

	Widget* findCommonAncestor(Widget* widget1, Widget* widget2);
	void unwindMouseOverStack(Widget* newTop);
	void unwindMouseOverStack();
	void doMouseInto(Widget* widget);
	void destroyFloater();
	void renderMouseCursor();

public:
	WidgetWindow();
	virtual ~WidgetWindow();

	static WidgetWindow* getInstance() { return instance; }

	void update();
	float getAnim() const { return anim; }
	virtual void clear();

	void registerUpdate(Widget* widget);
	void unregisterUpdate(Widget* widget);

	void setFloatingWidget(Widget* floater, bool modal = false);
	void removeFloatingWidget(Widget* floater);

	void aquireKeyboardFocus(KeyboardWidget* widget);
	void releaseKeyboardFocus(KeyboardWidget* widget);

	void setMouseCursorIcon(const Texture2D *tex = 0) { 
		mouseIcon = tex; 
		if (!tex) { 
			setMouseAppearance();
		}
	}
	void setMouseAppearance(MouseAppearance v = MouseAppearance::DEFAULT);
	bool loadMouse(const string &dir);
	void initMouse();

protected: // Shared::Platform::Window virtual events
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton);
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton);
	virtual void eventMouseMove(int x, int y, const MouseState &mouseState);
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton);
	virtual void eventMouseWheel(int x, int y, int zDelta);
	virtual void eventKeyDown(const Key &key);
	virtual void eventKeyUp(const Key &key);
	virtual void eventKeyPress(char c);

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
	virtual Vec2i getPrefSize() const	{ return Vec2i(-1);			}
	virtual Vec2i getMinSize() const	{ return Vec2i(800, 600);	}
	virtual Vec2i getMaxSize() const	{ return Vec2i(-1);			}

	TextRenderer* getTextRenderer() {
		return textRendererFT;
	}

	virtual void render();

	virtual string desc() { return string("[Window: ") + descPosDim() + "]"; }
};

#define g_widgetWindow (*WidgetWindow::getInstance())

}}

#endif
