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

#include "leak_dumper.h"

using Shared::Platform::WindowGl;
using Shared::Graphics::Gl::Texture2DGl;
using namespace Glest::Global;
using Glest::Graphics::Renderer;

namespace Glest { namespace Widgets {
using Global::CoreData;

// =====================================================
// class WidgetWindow
// =====================================================

WidgetWindow* WidgetWindow::instance;

WidgetWindow::WidgetWindow()
		: Container(this)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, floatingWidget(0)
		, anim(0.f), slowAnim(0.f), mouseIcon(0)
		, mouseMain(0), mouseAnimations(0) {
	size.x = Metrics::getInstance().getScreenW();
	size.y = Metrics::getInstance().getScreenH();
	
	mouseOverStack.push(this);

	foreach_enum (MouseButton, btn) {
		mouseDownWidgets[btn] = 0;
	}
	lastMouseDownWidget = 0;
	lastKeyDownWidget = 0;
	keyboardFocused = keyboardWidget;

	textRendererFT = g_renderer.getFreeTypeRenderer();
}

WidgetWindow::~WidgetWindow() {
	clear();
}

void WidgetWindow::clear() {
	//_PROFILE_FUNCTION();
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	keyboardFocused = keyboardWidget;
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
		if (keyboardFocused != keyboardWidget) {
			keyboardFocused->lostKeyboardFocus();
		}
		keyboardFocused = widget;
	}
}

void WidgetWindow::releaseKeyboardFocus(KeyboardWidget* widget) {
	if (keyboardFocused == widget) {
		keyboardFocused->lostKeyboardFocus();
		keyboardFocused = keyboardWidget;
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

void WidgetWindow::setFloatingWidget(Widget* floater, bool modal) {
	delete floatingWidget;
	floatingWidget = floater;
	floatingWidget->setParent(this);
	unwindMouseOverStack(this);
	modalFloater = modal;
}

void WidgetWindow::removeFloatingWidget(Widget* floater) {
	if (floater != floatingWidget) {
		throw runtime_error("WidgetWindow::removeFloatingWidget() passed bad argument.");
	}
	keyboardFocused = keyboardWidget;
	lastKeyDownWidget = 0;

	toClean.push_back(floatingWidget);
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
	///@todo it might be better to use a set instead of a vector to achieve the desired behaviour
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

	if (mouseAnimations) {
		mouseAnimations->update(); // shouldn't this be handled by the above? - hailstone 2Jan2011
	}
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
	mousePos.x = x;
	mousePos.y = getH() - y;
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
	if (lastMouseDownWidget && lastMouseDownWidget != widget->asMouseWidget()) {
		Widget* ancestor = findCommonAncestor(lastMouseDownWidget->me, widget);
		unwindMouseOverStack(ancestor);
		doMouseInto(widget);
	}
	if (keyboardFocused != keyboardWidget && keyboardFocused != widget->asKeyboardWidget()) {
		keyboardFocused->lostKeyboardFocus();
		keyboardFocused = keyboardWidget;
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
	mousePos.x = x;
	mousePos.y = getH() - y;
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
	mousePos.x = x;
	mousePos.y = getH() - y;

	if (mouseMain) {
		mouseMain->setPos(mousePos + Vec2i(0, -32));
	}

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
	mousePos.x = x;
	mousePos.y = getH() - y;
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

	Widget* widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		}
	} else {
		widget = getWidgetAt(mousePos);
	}
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

void WidgetWindow::initMouse() {
	remChild(mouseMain);
	mouseMain = new Imageset(this, g_coreData.getMouseTexture(), 32, 32);
	mouseMain->setSize(32,32);
	mouseMain->setPos(mousePos + Vec2i(0, -32));
	//mouseAnimations = new Animset(this, mouseMain, 30);
}

void WidgetWindow::renderMouseCursor() {
	float color1, color2;
	Vec2i points[4];
	points[0] = mousePos;
	points[1] = mousePos + Vec2i(20, -10);
	points[2] = mousePos + Vec2i(10, -20);

	int numPoints;
	if (mouseIcon) {
		mouseMain->setActive(1);
		numPoints = 4;
		points[3] = points[2];
		points[2] = mousePos + Vec2i(10, -10);
	} else {
		numPoints = 3;
	}

	int mAnim = int(slowAnim * 200) - 100;
	color2 = float(abs(mAnim)) / 100.f / 2.f + 0.4f;
	color1 = float(abs(mAnim)) / 100.f / 2.f + 0.8f;

	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
		glEnable(GL_BLEND);

		// hardcoded if texture doesn't exist
		if (mouseMain == NULL) {
			//inside
			glColor4f(0.4f, 0.2f, 0.2f, 0.5f);
			glBegin(GL_TRIANGLE_FAN);
			for (int i=0; i < numPoints; ++i) {
				glVertex2iv(points[i].ptr());
			}
			glEnd();

			//border
			glLineWidth(2);
			glBegin(GL_LINE_LOOP);
				glColor4f(1.f, 0.2f, 0, color1);
				glVertex2iv(points[0].ptr());
				glColor4f(1.f, 0.4f, 0, color2);
				for (int i=1; i < numPoints; ++i) {
					glVertex2iv(points[i].ptr());
				}
			glEnd();
		} else {
			// this is rendering it twice but is neccessary since the widget
			// render order isn't guaranteed
			mouseMain->render();
 		}

		if (mouseIcon) {
			int x1 = points[2].x + 1;
			int y2 = points[2].y - 1;
			int x2 = x1 + 32;
			int y1 = y2 - 32;
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(mouseIcon)->getHandle());
			glColor4f(1.f, 1.f, 1.f, 0.5f);
			glBegin(GL_TRIANGLE_STRIP);
				glTexCoord2i(0, 1);
				glVertex2i(x1, y2);
				glTexCoord2i(0, 0);
				glVertex2i(x1, y1);
				glTexCoord2i(1, 1);
				glVertex2i(x2, y2);
				glTexCoord2i(1, 0);
				glVertex2i(x2, y1);
			glEnd();
		}
	glPopAttrib();
}

}}

