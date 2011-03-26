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

TickerTape::TickerTape(Container *parent, Origin origin, Alignment alignment)
		: StaticText(parent)
		, m_origin(origin)
		, m_align(alignment)
		, m_transFunc(TransitionFunc::LINEAR)
		, m_alternateOrigin(false)
		, m_overlapTransitions(false)
		, m_displayInterval(240)
		, m_transitionInterval(120)
		, m_actionCounter(0)
		/*, m_currentIndex(-1)*/ {
	setWidgetStyle(WidgetType::TICKER_TAPE);
}

void TickerTape::setPositions(TextAction &action) {
	if (m_align == Alignment::CENTERED) {
		TextWidget::centreText(action.m_targetIndex);
	} else {
		const FontMetrics *fm = getFont(m_textStyle.m_fontIndex)->getMetrics();
		Vec2i dims = Vec2i(fm->getTextDiminsions(getText(action.m_targetIndex)));
		Vec2i size = getSize() - getBordersAll();
		switch (m_align) {
			case Alignment::JUSTIFIED:
				assert(false);
				break;
			case Alignment::FLUSH_LEFT:
				TextWidget::setTextPos(Vec2i(getBorderLeft(), (size.h - dims.h) / 2), action.m_targetIndex);
				break;
			case Alignment::FLUSH_RIGHT:
				Vec2i pos(size.w - getBorderRight() - dims.w, (size.h - dims.h) / 2);
				TextWidget::setTextPos(pos, action.m_targetIndex);
				break;
		}
	}
	Vec2i p = TextWidget::getTextPos(action.m_targetIndex);
	Vec2f targetPos = Vec2f(p);

	Origin origin = m_origin;
	if (m_alternateOrigin && action.m_actionNumber % 2 == 1) {
		switch (m_origin) {
			case Origin::FROM_TOP: origin = Origin::FROM_BOTTOM; break;
			case Origin::FROM_BOTTOM: origin = Origin::FROM_TOP; break;
			case Origin::FROM_LEFT: origin = Origin::FROM_RIGHT; break;
			case Origin::FROM_RIGHT: origin = Origin::FROM_LEFT; break;
			default: assert(false);
		}
	}
	if (action.m_phaseNumber == 0) {
		action.m_destPos = targetPos;
		if (origin == Origin::FROM_TOP) {
			action.m_startPos = targetPos + Vec2f(0.f, float(-getHeight()));
		} else if (origin == Origin::FROM_BOTTOM) {
			action.m_startPos = targetPos + Vec2f(0.f, float(getHeight()));
		} else if (origin == Origin::CENTRE) {
			action.m_startPos = targetPos;
		} else if (origin == Origin::FROM_LEFT) {
			action.m_startPos = targetPos + Vec2f(float(-getWidth()), 0.f);
		} else if (origin == Origin::FROM_RIGHT) {
			action.m_startPos = targetPos + Vec2f(float(getWidth()), 0.f);
		}
	} else if (action.m_phaseNumber == 1) {
		action.m_startPos = action.m_destPos = targetPos;
	} else if (action.m_phaseNumber == 2) {
		action.m_startPos = targetPos;
		if (origin == Origin::FROM_TOP) {
			action.m_destPos = targetPos + Vec2f(0.f, float(getHeight()));
		} else if (origin == Origin::FROM_BOTTOM) {
			action.m_destPos = targetPos + Vec2f(0.f, float(-getHeight()));
		} else if (origin == Origin::CENTRE) {
			action.m_destPos = targetPos;
		} else if (origin == Origin::FROM_LEFT) {
			action.m_destPos = targetPos + Vec2f(float(getWidth()), 0.f);
		} else if (origin == Origin::FROM_RIGHT) {
			action.m_destPos = targetPos + Vec2f(float(-getWidth()), 0.f);
		}
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

