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

#include "metrics.h"
#include "renderer.h"
#include "core_data.h"

using Shared::Platform::WindowGl;
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
		, anim(0.f), slowAnim(0.f) {
	size.x = Metrics::getInstance().getScreenW();
	size.y = Metrics::getInstance().getScreenH();
	cout << "WidgetWindow created, " << size.x << " x " << size.y << endl;

	mouseOverStack.push(this);

	foreach_enum (MouseButton, btn) {
		mouseDownWidgets[btn] = 0;
	}
	lastMouseDownWidget = 0;
	keyboardFocused = keyboardWidget;

	textRendererBM = g_renderer.getTextRenderer();
	textRendererFT = g_renderer.getFreeTypeRenderer();

}

WidgetWindow::~WidgetWindow() {
	clear();
}

void WidgetWindow::clear() {
	_PROFILE_FUNCTION();
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	keyboardFocused = keyboardWidget;
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
	Container::render();
	if (floatingWidget) {
		floatingWidget->render();
	}
	renderMouseCursor();
}

void WidgetWindow::aquireKeyboardFocus(KeyboardWidget::Ptr widget) {
	assert(widget);
	keyboardFocused = widget;
}

void WidgetWindow::releaseKeyboardFocus(KeyboardWidget::Ptr widget) {
	if (keyboardFocused == widget) {
		keyboardFocused = keyboardWidget;
	}
}

void WidgetWindow::unwindMouseOverStack(Widget::Ptr newTop) {
	while (mouseOverStack.top() != newTop) {
		MouseWidget::Ptr mw = mouseOverStack.top()->asMouseWidget();
		if (mw) {
			mw->EW_mouseOut();
		}
		mouseOverStack.pop();
	}
}

void WidgetWindow::unwindMouseOverStack() {
	unwindMouseOverStack(this);
}

Widget::Ptr WidgetWindow::findCommonAncestor(Widget::Ptr widget1, Widget::Ptr widget2) {
	Widget::Ptr tmp1 = widget1;
	while (tmp1 != this) {
		Widget::Ptr tmp2 = widget2;
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

void WidgetWindow::doMouseInto(Widget::Ptr widget) {
	if (widget != mouseOverStack.top()) {
		Widget::Ptr ancestor = findCommonAncestor(widget, mouseOverStack.top());
		unwindMouseOverStack(ancestor);
		std::stack<Widget::Ptr> tmpStack;
		while (widget != ancestor) {
			tmpStack.push(widget);
			widget = widget->getParent();
			assert(widget);
		}
		while (!tmpStack.empty()) {
			MouseWidget::Ptr mw = tmpStack.top()->asMouseWidget();
			if (mw) {
				mw->EW_mouseIn();
			}
			mouseOverStack.push(tmpStack.top());
			tmpStack.pop();
		}
	}
}

void WidgetWindow::setFloatingWidget(Widget::Ptr floater, bool modal) {
	delete floatingWidget;
	floatingWidget = floater;
	floatingWidget->setParent(this);
	unwindMouseOverStack(this);
	modalFloater = modal;
}

void WidgetWindow::removeFloatingWidget(Widget::Ptr floater) {
	if (floater != floatingWidget) {
		throw runtime_error("WidgetWindow::removeFloatingWidget() passed bad argument.");
	}
	toClean.push_back(floatingWidget);
	floatingWidget = 0;
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	doMouseInto(getWidgetAt(mousePos));
}

void WidgetWindow::registerUpdate(Widget::Ptr widget) {
	assert(std::find(updateList.begin(), updateList.end(), widget) == updateList.end());
	updateList.push_back(widget);
}

void WidgetWindow::unregisterUpdate(Widget::Ptr widget) {
	WidgetList::iterator it = std::find(updateList.begin(), updateList.end(), widget);
	if (it == updateList.end()) {
		assert(false);
		return;
	}
	updateList.erase(it);
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
}

//void WidgetWindow::setFade(float v) {
//	Container::setFade(v);
//	static float lastFade = 0.f;
//	if (v > lastFade + 0.05f || v < lastFade - 0.05f) {
//		DEBUG_HOOK();
//	}
//	lastFade = v;
//}

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
	mousePos.x = x;
	mousePos.y = getH() - y;
	Widget::Ptr widget = 0;
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
		Widget::Ptr ancestor = findCommonAncestor(lastMouseDownWidget->me, widget);
		unwindMouseOverStack(ancestor);
		doMouseInto(widget);
	}
	if (keyboardFocused != keyboardWidget && keyboardFocused != widget->asKeyboardWidget()) {
		keyboardFocused->EW_lostKeyboardFocus();
		keyboardFocused = keyboardWidget;
	}
	while (widget) {
		MouseWidget::Ptr mw = widget->asMouseWidget();
		if (mw && mw->EW_mouseDown(msBtn, mousePos)) {
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
	mousePos.x = x;
	mousePos.y = getH() - y;
	MouseWidget::Ptr downWidget = mouseDownWidgets[msBtn];
	if (downWidget) {
		downWidget->EW_mouseUp(msBtn, mousePos);
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
	assert(!mouseOverStack.empty());
	mousePos.x = x;
	mousePos.y = getH() - y;

	Widget::Ptr widget = 0;
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
		lastMouseDownWidget->EW_mouseMove(mousePos);
	} else {
		doMouseInto(widget);
		while (widget) {
			if (widget->asMouseWidget()) {
				if (widget->asMouseWidget()->EW_mouseMove(mousePos)) {
					return;
				}
			}
			widget = widget->getParent();
		}
	}
}

void WidgetWindow::eventMouseDoubleClick(int x, int y, MouseButton msBtn) {
	mousePos.x = x;
	mousePos.y = getH() - y;
	Widget::Ptr widget = 0;
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
		if (widget->asMouseWidget()
		&& widget->asMouseWidget()->EW_mouseDoubleClick(msBtn, mousePos)) {
			return;
		}
		widget = widget->getParent();
	}
}

void WidgetWindow::eventMouseWheel(int x, int y, int zDelta) {
	mousePos.x = x;
	mousePos.y = getH() - y;
	Widget::Ptr widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		}
	} else {
		widget = getWidgetAt(mousePos);
	}
	while (widget) {
		if (widget->asMouseWidget()
		&& widget->asMouseWidget()->EW_mouseWheel(mousePos, zDelta)) {
			return;
		}
		widget = widget->getParent();
	}
}

void WidgetWindow::eventKeyDown(const Key &key) {
	keyboardFocused->EW_keyDown(key);
}

void WidgetWindow::eventKeyUp(const Key &key) {
	keyboardFocused->EW_keyUp(key);
}

void WidgetWindow::eventKeyPress(char c) {
	keyboardFocused->EW_keyPress(c);
}

void WidgetWindow::renderMouseCursor() {
	float color1, color2;

	int mAnim = int(slowAnim * 200) - 100;
	color2 = float(abs(mAnim)) / 100.f / 2.f + 0.4f;
	color1 = float(abs(mAnim)) / 100.f / 2.f + 0.8f;

	Vec2i aPos = mousePos;// + Vec2i(25,0);

	glPushAttrib(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
		glEnable(GL_BLEND);

		//inside
		glColor4f(0.4f, 0.2f, 0.2f, 0.5f);
		glBegin(GL_TRIANGLES);
			glVertex2i(aPos.x, aPos.y);
			glVertex2i(aPos.x+20, aPos.y-10);
			glVertex2i(aPos.x+10, aPos.y-20);
		glEnd();

		//border
		glLineWidth(2);
		glBegin(GL_LINE_LOOP);
			glColor4f(1.f, 0.2f, 0, color1);
			glVertex2i(aPos.x, aPos.y);
			glColor4f(1.f, 0.4f, 0, color2);
			glVertex2i(aPos.x+20, aPos.y-10);
			glColor4f(1.f, 0.4f, 0, color2);
			glVertex2i(aPos.x+10, aPos.y-20);
		glEnd();
	glPopAttrib();
}

}}

