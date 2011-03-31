// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "ticker_tape.h"

namespace Glest { namespace Widgets {

TickerTape::TickerTape(Container *parent, SizeHint anchor, Alignment alignment)
		: StaticText(parent)
		, m_anchor(anchor)
		, m_align(alignment)
		, m_startOffset(0.f)
		, m_endOffset(0.f)
		, m_transFunc(TransitionFunc::LINEAR)
		, m_alternateOrigin(false)
		, m_overlapTransitions(false)
		, m_displayInterval(240)
		, m_transitionInterval(120)
		, m_actionCounter(0)
		/*, m_currentIndex(-1)*/ {
	setWidgetStyle(WidgetType::TICKER_TAPE);

}

inline Vec2f textDims(Widget *widget, const string &str) {
	return widget->getFont()->getMetrics()->getTextDiminsions(str);
}

void TickerTape::setPositions(TextAction &action) {
	Vec2i txtDims = Vec2i(textDims(this, getText(action.m_targetIndex)));
	Vec2i widgetSize = getSize() - getBordersAll();
	int yPos = (widgetSize.h - txtDims.h) / 2;
	int aPos;
	if (m_anchor.isPercentage()) {
		if (m_anchor.getPercentage() == -1) {
			aPos = widgetSize.w;
		} else {
			aPos = int(m_anchor.getPercentage() / 100.f * widgetSize.w);
		}
	} else {
		aPos = m_anchor.getAbsolute();
	}
	if (m_align == Alignment::CENTERED) {
		aPos -= txtDims.w / 2;
	} else if (m_align == Alignment::FLUSH_LEFT) {
		// nop
	} else if (m_align == Alignment::FLUSH_RIGHT) {
		aPos -= txtDims.w;
	} else {
		assert(false);
	}
	Vec2i destPos(getBorderLeft() + aPos, getBorderTop() + yPos);
	TextWidget::setTextPos(destPos, action.m_targetIndex);
	Vec2f targetPos = Vec2f(destPos);	
	Vec2f startOffset, endOffset;
	if (m_alternateOrigin && action.m_actionNumber % 2 == 1) {
		startOffset = m_endOffset;
		endOffset = m_startOffset;
	} else {
		startOffset = m_startOffset;
		endOffset = m_endOffset;
	}
	if (action.m_phaseNumber == 0) {
		action.m_startPos = targetPos + startOffset;
		action.m_destPos = targetPos;
	} else if (action.m_phaseNumber == 1) {
		action.m_startPos = action.m_destPos = targetPos;
	} else if (action.m_phaseNumber == 2) {
		action.m_startPos = targetPos;
		action.m_destPos = targetPos + endOffset;
	}
	setTextPos(Vec2i(action.m_startPos), action.m_targetIndex);
	setTextFade(action.m_startAlpha, action.m_targetIndex);
}

void TickerTape::startAction(int ndx) {
	m_actions.push_back(TextAction(m_transitionInterval, m_actionCounter++, this, ndx));
	TextAction &action = m_actions.back();
	action.m_transFuncPos = m_transFunc;
	action.m_transFuncAlpha = m_transFunc;
	action.m_phaseNumber = 0;
	setPositions(action);
	action.m_startAlpha = 0.f;
	action.m_destAlpha = 1.f;
	setTextFade(0.f, ndx);
}

void TickerTape::startTicker() {
	m_rootWindow->registerUpdate(this);
	startAction(0);
}

void TickerTape::setSize(const Vec2i &sz) {
	StaticText::setSize(sz);
	foreach (Actions, a, m_actions) {
		setPositions(*a);
	}
}

void TickerTape::addItems(const vector<string> &strings) {
	foreach_const (vector<string>, it, strings) {
		TextWidget::addText(*it);
	}
}

void TickerTape::addItems(const string *strings, unsigned n) {
	for (int i=0; i  < n; ++i) {
		TextWidget::addText(strings[i]);
	}
}

void TickerTape::addItems(const char **strings) {
	int i = 0;
	while (strings[i]) {
		TextWidget::addText(strings[i++]);
	}
}

void TickerTape::addItems(const char **strings, unsigned n) {
	for (int i=0; i < n; ++i) {
		TextWidget::addText(strings[i]);
	}
}

Vec2i TickerTape::getMinSize() const {
	Vec2i txtDim = getTextDimensions(0);
	Vec2i xtra = getBordersAll();
	for (int i=1; i < TextWidget::numSnippets(); ++i) {
		Vec2i dim = getTextDimensions(1);
		if (dim.w > txtDim.w) {
			txtDim.w = dim.w;
		}
		if (dim.h > txtDim.h) {
			txtDim.h = dim.h;
		}
	}
	return txtDim + xtra;
}

void TickerTape::render() {
	Widget::render();
	m_rootWindow->pushClipRect(getScreenPos() + Vec2i(getBorderLeft(), getBorderTop()), getSize() - getBordersAll());
	if (numSnippets() == 1 && m_displayInterval == -1) {
		TextWidget::renderText(0);
	} else {
		foreach (Actions, a, m_actions) {
			TextWidget::renderText(a->m_targetIndex);
		}
	}
	m_rootWindow->popClipRect();
}

void TickerTape::update() {
	int startNdx = -1;
	Actions::iterator a = m_actions.begin();
	while (a != m_actions.end()) {
		bool rem = false;
		if (a->update()) {
			if (m_displayInterval == -1) {
				DEBUG_HOOK();
			}
			if (m_displayInterval != -1
			&& ((a->m_phaseNumber == 1 && m_overlapTransitions)
			|| (a->m_phaseNumber == 2 && !m_overlapTransitions)))  {
				startNdx = (a->m_targetIndex + 1) % numSnippets();
			}
			if (a->m_phaseNumber == 2 || m_displayInterval == -1) {
				rem = true;
			} else {
				a->reset();
				a->m_phaseNumber += 1;
				a->m_length = a->m_phaseNumber == 1 ? m_displayInterval : m_transitionInterval;
				if (a->m_phaseNumber == 2) {
					a->m_inverse = true;
					a->m_startAlpha = 1.f;
					a->m_destAlpha = 0.f;
				} else {
					a->m_startAlpha = 1.f;
					a->m_destAlpha = 1.f;
				}
				setPositions(*a);
			}
		}
		if (rem) {
			a = m_actions.erase(a);
		} else {
			++a;
		}
	}
	if (startNdx != -1) {
		startAction(startNdx);
	}
}

}}

