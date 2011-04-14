// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
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
#include "imageset.h"
#include "platform_util.h"
#include "opengl.h"

#include "leak_dumper.h"

using namespace Shared::Graphics::Gl;
using namespace Shared::Platform;
using Glest::Graphics::Imageset;

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
		, anim(0.f), slowAnim(0.f)
		/*, mouseIcon(0)
		, mouseMain(0)
		, mouseAnimations(0) */{
	m_size = g_metrics.getScreenDims();

	// Window
	Window::setText("Glest Advanced Engine");
	Window::setStyle(g_config.getDisplayWindowed() ? wsWindowedFixed: wsFullscreen);
	Window::setPos(0, 0);
	Window::setSize(g_config.getDisplayWidth(), g_config.getDisplayHeight());
	Window::create();

	// set video mode
	setDisplaySettings();

	// some flags for model interpolation/rendering
	string lerpMethodName = g_config.getRenderInterpolationMethod();
	if (lerpMethodName == "x87") {
		Shared::Graphics::meshLerpMethod = LerpMethod::x87;
	} else if (lerpMethodName == "SIMD") {
		Shared::Graphics::meshLerpMethod = LerpMethod::SIMD;
	//} else if (lerpMethodName == "GLSL") {
	//	Shared::Graphics::meshLerpMethod = LerpMethod::GLSL;
	} else {
		// error ?
		Shared::Graphics::meshLerpMethod = LerpMethod::SIMD;
	}
	Shared::Graphics::use_vbos = g_config.getRenderUseVBOs();
	
	// render
	g_logger.logProgramEvent("Initialising OpenGL");
	initGl(g_config.getRenderColorBits(), g_config.getRenderDepthBits(), 0);
	makeCurrentGl();

	g_logger.logProgramEvent("Loading place-holder texture.");
	Texture2D::defaultTexture = g_renderer.getTexture2D(ResourceScope::GLOBAL, "data/core/misc_textures/default.tga");

	// load coreData & widgetConfig, (needs renderer, but must load before renderer init) and init renderer
	g_logger.logProgramEvent("Loading Widget config.");
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
	m_mouseCursor->render();
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
	assert(newTop != 0);
	while (mouseOverStack.top() != newTop) {
		MouseWidget* mw = mouseOverStack.top()->asMouseWidget();
		if (mw) {
			mw->mouseOut();
		}
		mouseOverStack.pop();
		assert(!mouseOverStack.empty());
	}
}

void WidgetWindow::unwindMouseOverStack() {
	unwindMouseOverStack(this);
}

void WidgetWindow::setScissor(const Rect2i &rect) {
	assert(glIsEnabled(GL_SCISSOR_TEST));
	Vec2i pos(rect.p[0].x, g_config.getDisplayHeight() - rect.p[1].y);
	Vec2i size(rect.p[1].x - rect.p[0].x, rect.p[1].y - rect.p[0].y);
	glScissor(pos.x, pos.y, size.w, size.h);
}

void WidgetWindow::pushClipRect(const Vec2i &pos, const Vec2i &size) {
	assertGl();
	Rect2i incoming = Rect2i(pos, pos + size);
	if (m_clipStack.empty()) {
		assert(!glIsEnabled(GL_SCISSOR_TEST));
		glEnable(GL_SCISSOR_TEST);
		setScissor(incoming);
		m_clipStack.push(incoming);
	} else {
		Rect2i existing = m_clipStack.top();
		Rect2i intersection = existing.interection(incoming);
		setScissor(intersection);
		m_clipStack.push(intersection);
	}
	assertGl();
}

void WidgetWindow::popClipRect() {
	assertGl();
	RUNTIME_CHECK(!m_clipStack.empty());
	m_clipStack.pop();
	if (m_clipStack.empty()) {
		assert(glIsEnabled(GL_SCISSOR_TEST));
		glDisable(GL_SCISSOR_TEST);
	} else {
		setScissor(m_clipStack.top());
	}

	assertGl();
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

void WidgetWindow::doMouseInto(Widget *widget) {
	assert(!mouseOverStack.empty());

	if (widget != mouseOverStack.top()) {
		Widget *ancestor = findCommonAncestor(widget, mouseOverStack.top());
		unwindMouseOverStack(ancestor);

		std::stack<Widget*> tmpStack;
		while (widget != ancestor) {
			tmpStack.push(widget);
			assert(widget->getParent() != 0);
			widget = widget->getParent();
			assert(widget != 0);
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
	WIDGET_LOG( __FUNCTION__ << "(0x" << intToHex(int(floater)) << ", " << (modal ? "true)" : "false)") );
	delete floatingWidget;
	floatingWidget = floater;
	unwindMouseOverStack(this);
	modalFloater = modal;
}

void WidgetWindow::removeFloatingWidget(Widget* floater) {
	WIDGET_LOG( __FUNCTION__ << "(0x" << intToHex(int(floater)) << ")" );
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
	// if not on the updateList or addUpdateQueue yet, add to addUpdateQueue
	if (!find(updateList, widget) && !find(addUpdateQueue, widget)) {
		addUpdateQueue.push_back(widget);
	}
}

void WidgetWindow::unregisterUpdate(Widget* widget) {
	WidgetList::iterator it;
	// check addUpdateQueue, in case it hasn't made it onto the updateList yet
	if (find(addUpdateQueue, widget, it)) {
		addUpdateQueue.erase(it);
	}
	// if it is in the updateList already, then put it on the remUpdateQueue
	if (find(updateList, widget, it)) {
		remUpdateQueue.push_back(*it);
	}
}

void WidgetWindow::update() {
	// update anim vars
	const float animSpeed = 0.01f;
	anim += animSpeed;
	if (anim > 1.f) {
		anim -= 1.f;
	}
	slowAnim += animSpeed / 3.f;
	if (slowAnim > 1.f) {
		slowAnim -= 1.f;
	}
	// delayed delete, stops iterators in siglot::signal objects going bad in emit() when 
	// the response to the signal is deletion of the source widget
	if (!toClean.empty()) {
		foreach (WidgetList, it, toClean) {
			delete *it;
		}
		toClean.clear();
	}

	// updateList: adding widgets to the update list, and removing them, is a delayed
	// action. If in response to an update() some other widget was added to the list, or
	// the one update()ing removed itself, the loop iterator wiould go bad, hence
	// the addUpdateQueue and remUpdateQueue

	// clean updateList
	foreach (WidgetList, it, remUpdateQueue) {
		WidgetList::iterator it2 = std::find(updateList.begin(), updateList.end(), *it);
		assert(it2 != updateList.end());
		updateList.erase(it2);
	}
	remUpdateQueue.clear();

	// call update() on everyone in updateList
	foreach (WidgetList, it, updateList) {
		(*it)->update();
	}

	// service any registerUpdate() requests 
	foreach (WidgetList, it, addUpdateQueue) {
		updateList.push_back(*it);
	}
	addUpdateQueue.clear();

	//if (mouseAnimations) {
	//	mouseAnimations->update(); // shouldn't this be handled by the above? - hailstone 2Jan2011. Yes?
	//}

	// check mouse over stack 10 times a second, to handle movement
	// of widgets in response to updates
	static int updateCount = 0;
	++updateCount;
	if (updateCount % (GameConstants::guiUpdatesPerSec / 10) == 0) {
		eventMouseMove(mousePos.x, mousePos.y, input.getMouseState());
	}
}

void WidgetWindow::destroyFloater() {
	WIDGET_LOG( __FUNCTION__ << "() : floatingWidget is @ 0x" << intToHex(int(floatingWidget)) );
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

Widget* WidgetWindow::getWidgetForMouseEvent() {
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
	return widget;
}

void WidgetWindow::eventMouseDown(int x, int y, MouseButton msBtn) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << ", " << MouseButtonNames[msBtn] << " )");
	mousePos = Vec2i(x, y);
	Widget* widget = getWidgetForMouseEvent();
	if (floatingWidget && widget == this) {
		if (!modalFloater) {
			destroyFloater();
		}
		return;
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
		doMouseInto(getWidgetForMouseEvent());
	}
}

void WidgetWindow::eventMouseMove(int x, int y, const MouseState &ms) {
	WIDGET_LOG( __FUNCTION__ << "( " << x << ", " << y << " )");
	assert(!mouseOverStack.empty());
	mousePos = Vec2i(x, y);
	m_mouseCursor->setPos(mousePos);

	if (lastMouseDownWidget) {
		if (isInside(mousePos)) {
			lastMouseDownWidget->mouseMove(mousePos);
		}
		return;
	}
	Widget* widget = getWidgetForMouseEvent();
	unwindMouseOverStack(findCommonAncestor(widget, mouseOverStack.top()));
	widget = getWidgetForMouseEvent();
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


//////////////////////
//////////////////////
//////////////////////

//REFACTOR: send to class ImageSetMouseCursor : public MouseCursor

/** Load the mouse texture from dir
  * @return true on success
  */
//bool WidgetWindow::loadMouse(const string &dir) {
//	string path;
//	string name = "mouse";
//
//	// check each file type (there must be a better way?)
//	if (fileExists(dir + "/" + name + ".png")) {
//		path = dir + "/" + name + ".png";
//	} else if (fileExists(dir + "/" + name + ".jpg")) {
//		path = dir + "/" + name + ".jpg";
//	} else if (fileExists(dir + "/" + name + ".bmp")) {
//		path = dir + "/" + name + ".bmp";
//	} else if (fileExists(dir + "/" + name + ".tga")) {
//		path = dir + "/" + name + ".tga";
//	} else {
//		return false;
//	}
//	
//	Texture2D *tex = g_renderer.newTexture2D(ResourceScope::GLOBAL);
//	tex->setMipmap(false);
//	tex->load(path);
//	tex->init();
//	
//	m_mouseTexture = tex;
//
//	return true;
//}
//
//void WidgetWindow::initMouse() {
//	if (!m_mouseTexture) {
//		m_mouseTexture = g_coreData.getMouseTexture();
//	}
//	delete mouseMain;
//	mouseMain = new Imageset(m_mouseTexture, 32, 32);
//	mouseMain->setSize(32,32);
//	mouseMain->setPos(mousePos + Vec2i(0, -32));
//	m_mouseTexture = 0; // custom mouse is only applied once
//	//mouseAnimations = new Animset(this, mouseMain, 30);
//}
//////////////////////
//////////////////////
//////////////////////

}}

