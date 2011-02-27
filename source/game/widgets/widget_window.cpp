// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include <stack>

#include "window_gl.h"
#include "sigslot.h"

#include "widgets_base.h"
#include "widget_window.h"
#include "widgets.h"

#include "metrics.h"
#include "renderer.h"
#include "core_data.h"
#include "texture_gl.h"
#include "platform_util.h"

#include "leak_dumper.h"

using Shared::Graphics::Gl::Texture2DGl;
using namespace Shared::Platform;

namespace Glest { namespace Widgets {
using namespace Global;
using namespace Graphics;

// WidgetWindow event logging...
#define ENABLE_WIDGET_LOGGING 0
// widget logging still needs to be turned on (in widgets_base.h), this just disables 
// the logging macros in this file.
#if !ENABLE_WIDGET_LOGGING
#	undef WIDGET_LOG
#	define WIDGET_LOG(x)
#endif

// =====================================================
// class CodeMouseCursor
// =====================================================

CodeMouseCursor::~CodeMouseCursor() {
	DEBUG_HOOK();
}

void CodeMouseCursor::setAppearance(MouseAppearance ma, const Texture2D *tex) {
	if (ma == MouseAppearance::CMD_ICON) {
		RUNTIME_CHECK(tex != 0);
		m_tex = tex;
		m_app = MouseAppearance::CMD_ICON;
	} else {
		m_tex = 0;
		m_app = MouseAppearance::DEFAULT;
	}
}

void CodeMouseCursor::render() {
	float color1, color2;
	Vec2i points[4];
	points[0] = getScreenPos();
	points[1] = points[0] + Vec2i(20, 10);
	points[2] = points[0] + Vec2i(10, 20);

	int numPoints;
	if (m_tex) {
		numPoints = 4;
		points[3] = points[2];
		points[2] = points[0] + Vec2i(10, 10);
	} else {
		numPoints = 3;
	}

	int mAnim = int(getRootWindow()->getSlowAnim() * 200) - 100;
	color2 = float(abs(mAnim)) / 100.f / 2.f + 0.4f;
	color1 = float(abs(mAnim)) / 100.f / 2.f + 0.8f;

	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);

	// inside
	glColor4f(0.4f, 0.2f, 0.2f, 0.5f);
	glBegin(GL_TRIANGLE_FAN);
	for (int i=0; i < numPoints; ++i) {
		glVertex2iv(points[i].ptr());
	}
	glEnd();

	// border
	glLineWidth(2);
	glBegin(GL_LINE_LOOP);
		glColor4f(1.f, 0.2f, 0, color1);
		glVertex2iv(points[0].ptr());
		glColor4f(1.f, 0.4f, 0, color2);
		for (int i=1; i < numPoints; ++i) {
			glVertex2iv(points[i].ptr());
		}
	glEnd();

	// command icon ?
	if (m_tex) {
		MouseCursor::renderTex(m_tex);
	}
	glPopAttrib();
}

// =====================================================
// class WidgetWindow
// =====================================================

WidgetWindow* WidgetWindow::instance;

WidgetWindow::WidgetWindow()
		: Container(this)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, floatingWidget(0)
		, anim(0.f), slowAnim(0.f)/*, mouseIcon(0)
		, mouseMain(0), mouseAnimations(0) */{
	m_size.x = Metrics::getInstance().getScreenW();
	m_size.y = Metrics::getInstance().getScreenH();

	// Window
	Window::setText("Glest Advanced Engine");
	Window::setStyle(g_config.getDisplayWindowed() ? wsWindowedFixed: wsFullscreen);
	Window::setPos(0, 0);
	Window::setSize(g_config.getDisplayWidth(), g_config.getDisplayHeight());
	Window::create();

	// set video mode
	setDisplaySettings();

	Shared::Graphics::use_simd_interpolation = g_config.getRenderInterpolateWithSIMD();
	
	// render
	initGl(g_config.getRenderColorBits(), g_config.getRenderDepthBits(), g_config.getRenderStencilBits());
	makeCurrentGl();

	Texture2D::defaultTexture = g_renderer.getTexture2D(ResourceScope::GLOBAL, "data/core/misc_textures/default.tga");

	// load coreData & widgetConfig, (needs renderer, but must load before renderer init) and init renderer
	g_widgetConfig.load();
	if (!g_coreData.load() || !g_renderer.init()) {
		throw runtime_error("An error occurred loading core data.\nPlease see glestadv-error.log");
	}
	m_config = &g_widgetConfig;

	mouseOverStack.push(this);

	foreach_enum (MouseButton, btn) {
		mouseDownWidgets[btn] = 0;
	}
	lastMouseDownWidget = 0;
	lastKeyDownWidget = 0;
	keyboardFocused = m_keyboardWidget;

	///@todo ImageSetMouseCursor ... & config option?
	if (true) {
		m_mouseCursor = new CodeMouseCursor(this);
	} else {
		//m_mouseCursor = new ImageSetMouseCursor(this);
	}

	textRendererFT = g_renderer.getFreeTypeRenderer();
} 

WidgetWindow::~WidgetWindow() {
	// delete children
	clear();

	//restore video mode
	restoreDisplaySettings();

}

void WidgetWindow::setDisplaySettings() {
	if (!g_config.getDisplayWindowed()) {
		int freq= g_config.getDisplayRefreshFrequency();
		int colorBits= g_config.getRenderColorBits();
		int screenWidth= g_config.getDisplayWidth();
		int screenHeight= g_config.getDisplayHeight();

		if (!(changeVideoMode(screenWidth, screenHeight, colorBits, freq)
		|| changeVideoMode(screenWidth, screenHeight, colorBits, 0))) {
			throw runtime_error( "Error setting video mode: " +
				intToStr(screenWidth) + "x" + intToStr(screenHeight) + "x" + intToStr(colorBits));
		}
	}
}

void WidgetWindow::restoreDisplaySettings(){
	if(!g_config.getDisplayWindowed()){
		restoreVideoMode();
	}
}

void WidgetWindow::clear() {
	//_PROFILE_FUNCTION();
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	keyboardFocused = m_keyboardWidget;
	lastKeyDownWidget = 0;
	foreach_enum (MouseButton, btn) {
		mouseDownWidgets[btn] = 0;
	}
	lastMouseDownWidget = 0;
	
	if (floatingWidget) {
		delete floatingWidget;
		floatingWidget = 0;
	}
	Container::clear();
}

void WidgetWindow::render() {
	//_PROFILE_FUNCTION();
	Container::render();
	if (floatingWidget) {
		floatingWidget->render();
	}
	renderMouseCursor();
}

void WidgetWindow::aquireKeyboardFocus(KeyboardWidget* widget) {
	assert(widget);
	if (keyboardFocused != widget) {
		if (keyboardFocused != m_keyboardWidget) {
			keyboardFocused->lostKeyboardFocus();
		}
		keyboardFocused = widget;
	}
}

void WidgetWindow::releaseKeyboardFocus(KeyboardWidget* widget) {
	if (keyboardFocused == widget) {
		keyboardFocused->lostKeyboardFocus();
		keyboardFocused = m_keyboardWidget;
	}
}

void WidgetWindow::unwindMouseOverStack(Widget* newTop) {
	while (mouseOverStack.top() != newTop) {
		MouseWidget* mw = mouseOverStack.top()->asMouseWidget();
		if (mw) {
			mw->mouseOut();
		}
		mouseOverStack.pop();
	}
}

void WidgetWindow::unwindMouseOverStack() {
	unwindMouseOverStack(this);
}

Widget* WidgetWindow::findCommonAncestor(Widget* widget1, Widget* widget2) {
	Widget* tmp1 = widget1;
	while (tmp1 != this) {
		Widget* tmp2 = widget2;
		while (tmp1 != tmp2 && tmp2 != this) {
			tmp2 = tmp2->getParent();
		}
		if (tmp1 == tmp2) {
			return tmp1;
		}
		tmp1 = tmp1->getParent();
	}
	return this;
}

void WidgetWindow::doMouseInto(Widget* widget) {
	if (widget != mouseOverStack.top()) {
		Widget* ancestor = findCommonAncestor(widget, mouseOverStack.top());
		RUNTIME_CHECK(ancestor != 0);
		unwindMouseOverStack(ancestor);
		std::stack<Widget*> tmpStack;
		while (widget != ancestor) {
			tmpStack.push(widget);
			widget = widget->getParent();
			RUNTIME_CHECK(widget != 0);
		}
		while (!tmpStack.empty()) {
			MouseWidget* mw = tmpStack.top()->asMouseWidget();
			if (mw) {
				mw->mouseIn();
			}
			mouseOverStack.push(tmpStack.top());
			tmpStack.pop();
		}
	}
}
//
//void WidgetWindow::setMouseAppearance(MouseAppearance v) {
//	mouseMain->setActive(v);
//}

void WidgetWindow::setFloatingWidget(Widget* floater, bool modal) {
	delete floatingWidget;
	floatingWidget = floater;
	unwindMouseOverStack(this);
	modalFloater = modal;
}

void WidgetWindow::removeFloatingWidget(Widget* floater) {
	if (floater != floatingWidget) {
		throw runtime_error("WidgetWindow::removeFloatingWidget() passed bad argument.");
	}
	keyboardFocused = m_keyboardWidget;
	lastKeyDownWidget = 0;

	toClean.push_back(floatingWidget);
	floatingWidget->setVisible(false);
	floatingWidget = 0;
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	doMouseInto(getWidgetAt(mousePos));
}

void WidgetWindow::registerUpdate(Widget* widget) {
	// only register for updating once, if the widget is already in the container do nothing
	if (std::find(updateList.begin(), updateList.end(), widget) == updateList.end()) {
		updateList.push_back(widget);
	}
	// /@ todo it might be better to use a set instead of a vector to achieve the desired behaviour
	// JM: set<> == fast look-up, not fast iterate,  better to iterate fast and do this as you have, 
	// registerUpdate() should not be called often
}

void WidgetWindow::unregisterUpdate(Widget* widget) {
	WidgetList::iterator it = std::find(updateList.begin(), updateList.end(), widget);
	if (it != updateList.end()) {
		updateList.erase(it);
	}
}

void WidgetWindow::update() {
	const float animSpeed = 0.02f;
	anim += animSpeed;
	if (anim > 1.f) {
		anim -= 1.f;
	}
	slowAnim += animSpeed / 3.f;
	if (slowAnim > 1.f) {
		slowAnim -= 1.f;
	}
	if (!toClean.empty()) {
		foreach (WidgetList, it, toClean) {
			delete *it;
		}
		toClean.clear();
	}
	foreach (WidgetList, it, updateList) {
		(*it)->update();
	}

	//if (mouseAnimations) {
	//	mouseAnimations->update(); // shouldn't this be handled by the above? - hailstone 2Jan2011
	//}
}

void WidgetWindow::destroyFloater() {
	// destroy floater
	delete floatingWidget;
	floatingWidget = 0;
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	doMouseInto(getWidgetAt(mousePos));
	if (lastMouseDownWidget) {
		lastMouseDownWidget = 0;
		mouseDownWidgets[lastMouseDownButton] = 0;
	}
}

void WidgetWindow::eventMouseDown(int x, int y, MouseButton msBtn) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << ", " << MouseButtonNames[msBtn] << " )");
	mousePos = Vec2i(x, y);
	Widget* widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		} else {
			if (!modalFloater) {
				destroyFloater();
			}
			return;
		}
	} else {
		widget = getWidgetAt(mousePos);
	}
	//cout << "eventMouseDown @" << mousePos << " on " << widget->desc() << endl;

	if (lastMouseDownWidget && lastMouseDownWidget != widget->asMouseWidget()) {
		Widget* ancestor = findCommonAncestor(lastMouseDownWidget->me, widget);
		unwindMouseOverStack(ancestor);
		doMouseInto(widget);
	}
	if (keyboardFocused != m_keyboardWidget && keyboardFocused != widget->asKeyboardWidget()) {
		keyboardFocused->lostKeyboardFocus();
		keyboardFocused = m_keyboardWidget;
	}
	while (widget) {
		MouseWidget* mw = widget->asMouseWidget();
		if (mw && mw->mouseDown(msBtn, mousePos)) {
			if (lastMouseDownWidget) {
				mouseDownWidgets[lastMouseDownButton] = 0;
			}
			mouseDownWidgets[msBtn] = lastMouseDownWidget = mw;
			lastMouseDownButton = msBtn;
			return;
		}
		widget = widget->getParent();
	}
	mouseDownWidgets[msBtn] = lastMouseDownWidget = 0;
}

void WidgetWindow::eventMouseUp(int x, int y, MouseButton msBtn) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << ", " << MouseButtonNames[msBtn] << " )");
	mousePos = Vec2i(x, y);
	MouseWidget* downWidget = mouseDownWidgets[msBtn];
	if (downWidget) {
		downWidget->mouseUp(msBtn, mousePos);
		mouseDownWidgets[msBtn] = 0;
	}
	if (lastMouseDownWidget == downWidget) {
		lastMouseDownWidget = 0;
		if (floatingWidget) {
			doMouseInto(floatingWidget->getWidgetAt(mousePos));
		} else {
			doMouseInto(getWidgetAt(mousePos));
		}
	}
}

void WidgetWindow::eventMouseMove(int x, int y, const MouseState &ms) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	assert(!mouseOverStack.empty());
	mousePos = Vec2i(x, y);
	m_mouseCursor->setPos(mousePos);

	Widget* widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		}
	} else {
		if (isInside(mousePos)) {
			widget = getWidgetAt(mousePos);
		}
	}
	if (!widget) {
		widget = this;
	}
	assert(widget);
	if (lastMouseDownWidget) {
		lastMouseDownWidget->mouseMove(mousePos);
	} else {
		doMouseInto(widget);
		while (widget) {
			if (widget->asMouseWidget()) {
				if (widget->asMouseWidget()->mouseMove(mousePos)) {
					return;
				}
			}
			widget = widget->getParent();
		}
	}
}

void WidgetWindow::eventMouseDoubleClick(int x, int y, MouseButton msBtn) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << ", " << MouseButtonNames[msBtn] << " )");
	mousePos = Vec2i(x, y);
	Widget* widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		} else {
			if (!modalFloater) {
				destroyFloater();
			}
			return;
		}
	} else {
		widget = getWidgetAt(mousePos);
	}
	while (widget) {
		MouseWidget *mw = widget->asMouseWidget();
		if (mw && mw->mouseDoubleClick(msBtn, mousePos)) {
			if (lastMouseDownWidget) {
				mouseDownWidgets[lastMouseDownButton] = 0;
			}
			mouseDownWidgets[msBtn] = lastMouseDownWidget = mw;
			lastMouseDownButton = msBtn;
			return;
		}
		widget = widget->getParent();
	}
}

void WidgetWindow::eventMouseWheel(int x, int y, int zDelta) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << ", " << zDelta << " )");
	mousePos = Vec2i(x, y);

	Widget* widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		}
	} else {
		widget = getWidgetAt(mousePos);
	}
	//cout << "eventMouseWheel @" << mousePos << " on " << widget->desc() << endl;
	while (widget) {
		if (widget->asMouseWidget()
		&& widget->asMouseWidget()->mouseWheel(mousePos, zDelta)) {
			return;
		}
		widget = widget->getParent();
	}
}

void WidgetWindow::eventKeyDown(const Key &key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	lastKeyDownWidget = keyboardFocused;
	keyboardFocused->keyDown(key);
}

void WidgetWindow::eventKeyUp(const Key &key) {
	WIDGET_LOG( __FUNCTION__ << "( " << Key::getName(KeyCode(key)) << " )");
	if (keyboardFocused == lastKeyDownWidget) {
		keyboardFocused->keyUp(key);
	}
}

void WidgetWindow::eventKeyPress(char c) {
	WIDGET_LOG( __FUNCTION__ << "( '" << c << "' )");
	if (keyboardFocused == lastKeyDownWidget) {
		keyboardFocused->keyPress(c);
	}
}

//void WidgetWindow::initMouse() {
//	remChild(mouseMain);
//	mouseMain = new Imageset(this, g_coreData.getMouseTexture(), 32, 32);
//	mouseMain->setSize(32,32);
//	mouseMain->setPos(mousePos);
//	//mouseAnimations = new Animset(this, mouseMain, 30);
//}

void WidgetWindow::renderMouseCursor() {
	m_mouseCursor->render();
}

}}

