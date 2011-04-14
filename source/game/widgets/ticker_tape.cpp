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

ActionBase::ActionBase(const int length)
		: m_length(length), m_counter(0), m_startPos(0.f), m_destPos(0.f)
		, m_startAlpha(1.f), m_destAlpha(1.f)
		, m_transFuncPos(TransitionFunc::LINEAR)
		, m_transFuncAlpha(TransitionFunc::LINEAR)
		, m_inverse(false) {
}

ActionBase& ActionBase::operator=(const ActionBase &that) {
	memcpy(this, &that, sizeof(ActionBase));
	return *this;
}

void ActionBase::setPosTransition(const Vec2f &start, const Vec2f &dest, TransitionFunc tf) {
	m_startPos = start;
	m_destPos = dest;
	m_transFuncPos = tf;
}

void ActionBase::setAlphaTransition(const float start, const float dest, TransitionFunc tf) {
	m_startAlpha = start;
	m_destAlpha = dest;
	m_transFuncAlpha = tf;
}

bool ActionBase::update() {
	++m_counter;
	if (m_counter < 0) {
		return false;
	}
	Vec2f pos;
	float alpha;
	const float two_e = 2.f * 2.71828183f;
	float tt = float(m_counter) / float(m_length);
	if (m_inverse) {
		tt = 1.f - tt;
	}
	if (m_startPos != m_destPos) {
		float t = tt;
		if (m_transFuncPos == TransitionFunc::SQUARED) {
			t *= t;
		} else if (m_transFuncPos == TransitionFunc::EXPONENTIAL) {
			t = exp(t * two_e - two_e);
		} else if (m_transFuncPos == TransitionFunc::SQUARE_ROOT) {
			t = sqrt(t);
		} else if (m_transFuncPos == TransitionFunc::LOGARITHMIC) {
			t = (log(t) + two_e) / two_e;
		}
		t = clamp(t, 0.f, 1.f);
		if (m_inverse) {
			pos = m_destPos.lerp(t, m_startPos);
		} else {
			pos = m_startPos.lerp(t, m_destPos);
		}
	} else {
		pos = m_startPos;
	}
	if (m_startAlpha != m_destAlpha) {
		float t = tt;
		if (m_transFuncAlpha == TransitionFunc::SQUARED) {
			t *= t;
		} else if (m_transFuncAlpha == TransitionFunc::EXPONENTIAL) {
			t = exp(t * two_e - two_e);
		} else if (m_transFuncPos == TransitionFunc::SQUARE_ROOT) {
			t = sqrt(t);
		} else if (m_transFuncAlpha == TransitionFunc::LOGARITHMIC) {
			t = (log(t) + two_e) / two_e;
		}
		t = clamp(t, 0.f, 1.f);
		if (m_inverse) {
			alpha = m_destAlpha + (m_startAlpha - m_destAlpha) * t;
		} else {
			alpha = m_startAlpha + (m_destAlpha - m_startAlpha) * t;
		}
	} else {
		alpha = m_startAlpha;
	}
	updateTarget(pos, alpha);
	if (m_counter == m_length) {
		//Finished(this);
		return true;
	}
	return false;
}

TextAction::TextAction(const int length, int16 number,  TextWidget *textWidget, int index)
		: ActionBase(length), m_targetWidget(textWidget)
		, m_actionNumber(number), m_phaseNumber(0), m_targetIndex(index) {
}

TextAction& TextAction::operator=(const TextAction &that) {
	memcpy(this, &that, sizeof(TextAction));
	return *this;
}

void TextAction::updateTarget(Vec2f pos, float alpha) {
	Vec2i cPos = m_targetWidget->getTextPos(m_targetIndex);
	float cAlpha = m_targetWidget->widget()->getFade();
	Vec2i nPos = Vec2i(pos);
	if (cPos != nPos) {
		m_targetWidget->setTextPos(nPos, m_targetIndex);
	}
	if (cAlpha != alpha) {
		m_targetWidget->setTextFade(alpha, m_targetIndex);
	}
}

MoveWidgetAction::MoveWidgetAction(const int length, Widget *widget)
		: ActionBase(length), m_targetWidget(widget) {
}

void MoveWidgetAction::updateTarget(Vec2f pos, float alpha) {
	Vec2i cPos = m_targetWidget->getPos();
	float cAlpha = m_targetWidget->getFade();
	Vec2i nPos = Vec2i(pos);
	if (cPos != nPos) {
		m_targetWidget->setPos(nPos);
	}
	if (cAlpha != alpha) {
		m_targetWidget->setFade(alpha);
	}
}

ResizeWidgetAction::ResizeWidgetAction(const int len, Widget *widget)
		: ActionBase(len), m_targetWidget(widget) {
}

void ResizeWidgetAction::updateTarget(Vec2f sz, float) {
	Vec2i cSz = m_targetWidget->getSize();
	Vec2i nSz = Vec2i(sz);
	if (cSz != nSz) {
		m_targetWidget->setSize(nSz);
	}
}


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

