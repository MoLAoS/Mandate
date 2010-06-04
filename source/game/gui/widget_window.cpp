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

using Shared::Platform::WindowGl;
using namespace Glest::Global;
using Glest::Graphics::Renderer;

namespace Glest { namespace Widgets {

// =====================================================
// class WidgetWindow
// =====================================================

WidgetWindow* WidgetWindow::instance;

WidgetWindow::WidgetWindow()
		: Widget(this)
		, floatingWidget(0)
		, anim(0.f)
		, layerIdCounter(0) {
	size.x = Metrics::getInstance().getScreenW();
	size.y = Metrics::getInstance().getScreenH();
	cout << "WidgetWindow created, " << size.x << " x " << size.y << endl;

	mouseOverStack.push(this);

	foreach_enum (MouseButton, btn) {
		mouseDownWidgets[btn] = 0;
	}
	lastMouseDownWidget = 0;

	textRendererBM = theRenderer.getTextRenderer();
	textRendererFT = theRenderer.getFreeTypeRenderer();

	rootLayer = new Layer(this, "root", layerIdCounter++);
	rootLayer->setSize(size);
	layerNames.insert("root");
	layers.push_front(rootLayer);
	children.push_back(rootLayer);
	rootLayer->setParent(this);

	keyboardFocused = rootLayer;
	mouseOverStack.push(rootLayer);
}

WidgetWindow::~WidgetWindow() {
	clear();
}

void WidgetWindow::clear() {
	while (mouseOverStack.top() != this) {
		mouseOverStack.pop();
	}
	keyboardFocused = this;
	foreach_enum (MouseButton, btn) {
		mouseDownWidgets[btn] = 0;
	}
	rootLayer->clear();
	Container::remChild(rootLayer);
	Container::clear();
	layers.clear();
	layers.push_front(rootLayer);
	layerNames.clear();
	layerNames.insert("root");
	Container::addChild(rootLayer);
}

void WidgetWindow::render() {
	LayerList::reverse_iterator it = layers.rbegin();
	LayerList::reverse_iterator itEnd = layers.rend();
	while (it != itEnd) {
		(*it)->render();
		++it;
	}
	if (floatingWidget) {
		floatingWidget->render();
	}
}

void WidgetWindow::aquireKeyboardFocus(WidgetPtr widget) {
	assert(widget);
	keyboardFocused = widget;
}

void WidgetWindow::releaseKeyboardFocus(WidgetPtr widget) {
	if (keyboardFocused == widget) {
		keyboardFocused = this;
	}
}

WidgetWindow::LayerList::iterator WidgetWindow::findLayer(int id) {
	foreach (LayerList, it, layers) {
		if ((*it)->getId() == id) {
			return it;
		}
	}
	return layers.end();
}

WidgetWindow::LayerList::iterator WidgetWindow::findLayer(const string &name) {
	foreach (LayerList, it, layers) {
		if ((*it)->getName() == name) {
			return it;
		}
	}
	return layers.end();
}


void WidgetWindow::changeActiveLayer(LayerList::iterator &it) {
	if (it == layers.end()) {
		throw runtime_error("Layer not found.");
	} else if (it == layers.begin()) {
		return;
	}
	LayerPtr ptr = *it;
	layers.erase(it);
	layers.push_front(ptr);
	unwindMouseOverStack();
	doMouseInto(ptr->getWidgetAt(mousePos));
}


void WidgetWindow::setActiveLayer(const string &layer) {
	LayerList::iterator it = findLayer(layer);
	changeActiveLayer(it);
}

void WidgetWindow::setActiveLayer(const int layer) {
	LayerList::iterator it = findLayer(layer);
	changeActiveLayer(it);
}

LayerPtr WidgetWindow::getLayer(const string &layer) {
	LayerList::iterator it = findLayer(layer);
	return (it != layers.end()) ? *it : 0;
}

LayerPtr WidgetWindow::getLayer(const int layer) {
	LayerList::iterator it = findLayer(layer);
	return (it != layers.end()) ? *it : 0;
}

LayerPtr WidgetWindow::addNewLayer(const string &name, bool activate) {
	if (layerNames.find(name) != layerNames.end()) {
		throw runtime_error("Error: duplicate layer names.");
	}
	LayerPtr ptr = new Layer(this, name, layerIdCounter++);
	ptr->setParent(this);
	ptr->setSize(size);
	layerNames.insert(name);
	layers.push_back(ptr);
	children.push_back(ptr);
	if (activate) {
		layers.push_front(ptr);
	} else {
		layers.push_back(ptr);
	}
	return ptr;
}

WidgetPtr WidgetWindow::getWidgetAt(const Vec2i &pos) {
	LayerPtr layer;
	foreach (LayerList, it, layers) {
		layer = *it;
		if (layer->isEnabled() && layer->isInside(pos)) {
			return layer->getWidgetAt(pos);
		}
	}
	return this;
}

void WidgetWindow::registerMouseWidget(WidgetPtr asWidget, MouseWidgetPtr asMouseWidget) {
	assert(mouseWidgetMap.find(asWidget) == mouseWidgetMap.end());
	mouseWidgetMap[asWidget] = asMouseWidget;
}

void WidgetWindow::unregisterMouseWidget(WidgetPtr mouseWidget) {
	assert(mouseWidgetMap.find(mouseWidget) != mouseWidgetMap.end());
	mouseWidgetMap.erase(mouseWidget);
}

MouseWidgetPtr WidgetWindow::asMouseWidget(WidgetPtr widget) {
	MouseWidgetMap::iterator it = mouseWidgetMap.find(widget);
	if (it == mouseWidgetMap.end()) {
		return 0;
	}
	return it->second;
}

void WidgetWindow::registerKeyboardWidget(WidgetPtr asWidget, KeyboardWidgetPtr asKeyboardWidget) {
	assert(keyboardWidgetMap.find(asWidget) == keyboardWidgetMap.end());
	keyboardWidgetMap[asWidget] = asKeyboardWidget;
}

void WidgetWindow::unregisterKeyboardWidget(WidgetPtr widget) {
	assert(keyboardWidgetMap.find(widget) != keyboardWidgetMap.end());
	keyboardWidgetMap.erase(widget);
}

KeyboardWidgetPtr WidgetWindow::asKeyboardWidget(WidgetPtr widget) {
	KeyboardWidgetMap::iterator it = keyboardWidgetMap.find(widget);
	if (it == keyboardWidgetMap.end()) {
		return 0;
	}
	return it->second;
}

void WidgetWindow::unwindMouseOverStack(WidgetPtr newTop) {
	while (mouseOverStack.top() != newTop) {
		MouseWidgetPtr mouseWidget = asMouseWidget(mouseOverStack.top());
		if (mouseWidget) {
			mouseWidget->EW_mouseOut();
		}
		mouseOverStack.top()->mouseOut();
		mouseOverStack.pop();
	}
}

void WidgetWindow::unwindMouseOverStack() {
	unwindMouseOverStack(this);
}

WidgetPtr WidgetWindow::findCommonAncestor(WidgetPtr widget1, WidgetPtr widget2) {
	WidgetPtr tmp1 = widget1;
	while (tmp1 != this) {
		WidgetPtr tmp2 = widget2;
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

void WidgetWindow::doMouseInto(WidgetPtr widget) {
	if (widget != mouseOverStack.top()) {
		WidgetPtr ancestor = findCommonAncestor(widget, mouseOverStack.top());
		unwindMouseOverStack(ancestor);
		std::stack<WidgetPtr> tmpStack;
		while (widget != ancestor) {
			tmpStack.push(widget);
			widget = widget->getParent();
			assert(widget);
		}
		while (!tmpStack.empty()) {
			MouseWidgetPtr mouseWidget = asMouseWidget(tmpStack.top());
			if (mouseWidget) {
				mouseWidget->EW_mouseIn();
			}
			tmpStack.top()->mouseIn();
			mouseOverStack.push(tmpStack.top());
			tmpStack.pop();
		}
	}
}

void WidgetWindow::setFloatingWidget(WidgetPtr floater) {
	delete floatingWidget;
	floatingWidget = floater;
	floatingWidget->setParent(this);
	unwindMouseOverStack(this);
}

void WidgetWindow::removeFloatingWidget(WidgetPtr floater) {
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

void WidgetWindow::registerUpdate(WidgetPtr widget) {
	assert(std::find(updateList.begin(), updateList.end(), widget) == updateList.end());
	updateList.push_back(widget);
}

void WidgetWindow::unregisterUpdate(WidgetPtr widget) {
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

void WidgetWindow::eventMouseDown(int x, int y, MouseButton msBtn) {
	mousePos.x = x;
	mousePos.y = getH() - y;
	WidgetPtr widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		} else {
			// destroy floater
			delete floatingWidget;
			floatingWidget = 0;
			doMouseInto(layers.front()->getWidgetAt(mousePos));
			return;
		}
	} else {
		widget = layers.front()->getWidgetAt(mousePos);
	}

	if (lastMouseDownWidget && lastMouseDownWidget != widget) {
		WidgetPtr ancestor = findCommonAncestor(lastMouseDownWidget, widget);
		unwindMouseOverStack(ancestor);
		doMouseInto(widget);
	}

	if (keyboardFocused != this && keyboardFocused != widget) {
		keyboardFocused->lostKeyboardFocus();
		keyboardFocused = this;
	}
	mouseDownWidgets[msBtn] = lastMouseDownWidget = widget;
	widget->mouseDown(msBtn, mousePos);
	
	while (widget) {
		MouseWidgetPtr mouseWidget = asMouseWidget(widget);
		if (mouseWidget) {
			if (mouseWidget->EW_mouseDown(msBtn, mousePos)) {
				return;
			}
		}
		widget = widget->getParent();
	}
}

void WidgetWindow::eventMouseUp(int x, int y, MouseButton msBtn) {
	mousePos.x = x;
	mousePos.y = getH() - y;
	WidgetPtr downWidget = mouseDownWidgets[msBtn];
	if (downWidget) {
		downWidget->mouseUp(msBtn, mousePos);
		mouseDownWidgets[msBtn] = 0;
	}
	if (lastMouseDownWidget == downWidget) {
		lastMouseDownWidget = 0;
		if (floatingWidget) {
			doMouseInto(floatingWidget->getWidgetAt(mousePos));
		} else {
			doMouseInto(layers.front()->getWidgetAt(mousePos));
		}
	}
}

///@todo a badly configured Gui can crash this pretty easily.. add sanity checks
void WidgetWindow::eventMouseMove(int x, int y, const MouseState &ms) {
	assert(!mouseOverStack.empty());
	mousePos.x = x;
	mousePos.y = getH() - y;

	WidgetPtr widget = 0;
	if (floatingWidget) {
		if (floatingWidget->isInside(mousePos)) {
			widget = floatingWidget->getWidgetAt(mousePos);
		}
	} else {
		if (layers.front()->isInside(mousePos)) {
			widget = layers.front()->getWidgetAt(mousePos);
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
		widget->mouseMove(mousePos);
	}
}

void WidgetWindow::eventMouseDoubleClick(int x, int y, MouseButton msBtn) {
	mousePos.x = x;
	mousePos.y = getH() - y;
	///@todo floatingWidget
	layers.front()->getWidgetAt(mousePos)->mouseDoubleClick(msBtn, mousePos);
}

void WidgetWindow::eventMouseWheel(int x, int y, int zDelta) {
	mousePos.x = x;
	mousePos.y = getH() - y;
	///@todo floatingWidget
	layers.front()->getWidgetAt(mousePos)->mouseWheel(mousePos, zDelta);
}

void WidgetWindow::eventKeyDown(const Key &key) {
	keyboardFocused->keyDown(key);
}

void WidgetWindow::eventKeyUp(const Key &key) {
	keyboardFocused->keyUp(key);
}

void WidgetWindow::eventKeyPress(char c) {
	keyboardFocused->keyPress(c);
}

}}

