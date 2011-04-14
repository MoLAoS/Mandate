// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_TICKER_TAPE_INCLUDED_
#define _GLEST_WIDGETS_TICKER_TAPE_INCLUDED_

#include <stack>

#include "widgets.h"
#include "widget_config.h"
#include "widget_window.h"

namespace Glest { namespace Widgets {

/** enum TransitionFunc
  * dictates how to interpolate a transition,
  * http://fooplot.com/index.php?&type0=0&type1=0&type2=0&type3=0&type4=0&y0=x&y1=%28ln%28x%29%20%2B%202%20*%20e%29%20/%20%282%20*%20e%29&y2=exp%28x%20*%202%20*%20e%20-%202%20*%20e%29&y3=x%5E%202&y4=sqrt%28x%29&r0=&r1=&r2=&r3=&r4=&px0=&px1=&px2=&px3=&px4=&py0=&py1=&py2=&py3=&py4=&smin0=0&smin1=0&smin2=0&smin3=0&smin4=0&smax0=2pi&smax1=2pi&smax2=2pi&smax3=2pi&smax4=2pi&thetamin0=0&thetamin1=0&thetamin2=0&thetamin3=0&thetamin4=0&thetamax0=2pi&thetamax1=2pi&thetamax2=2pi&thetamax3=2pi&thetamax4=2pi&ipw=0&ixmin=-5&ixmax=5&iymin=-3&iymax=3&igx=0.1&igy=0.1&igl=1&igs=0&iax=1&ila=1&xmin=-0.5967266807957499&xmax=1.4285407550982856&ymin=-0.4715617275247235&ymax=1.5650782844026772
  */
WRAPPED_ENUM( TransitionFunc, LINEAR, SQUARED, EXPONENTIAL, SQUARE_ROOT, LOGARITHMIC );

struct ActionBase {
	int            m_length;
	int            m_counter;
	Vec2f          m_startPos;
	Vec2f          m_destPos;
	float          m_startAlpha;
	float          m_destAlpha;
	TransitionFunc m_transFuncPos;
	TransitionFunc m_transFuncAlpha;
	bool           m_inverse;

	ActionBase(const int length);
	ActionBase& operator=(const ActionBase &that);

	void setPosTransition(const Vec2f &start, const Vec2f &dest, TransitionFunc tf);
	void setAlphaTransition(const float start, const float dest, TransitionFunc tf);
	void reset() { m_counter = 0; }

	virtual void updateTarget(Vec2f pos, float alpha) = 0;

	bool update();
};

struct TextAction : public ActionBase {
	TextWidget*    m_targetWidget;
	int16          m_actionNumber;
	int16          m_phaseNumber;
	int            m_targetIndex;

	TextAction(const int length, int16 number,  TextWidget *textWidget, int index);
	TextAction& operator=(const TextAction &that);

	virtual void updateTarget(Vec2f pos, float alpha) override;
};

struct MoveWidgetAction : public ActionBase {
	Widget*        m_targetWidget;

	MoveWidgetAction(const int length, Widget *widget);
	virtual void updateTarget(Vec2f pos, float alpha) override;
};

struct ResizeWidgetAction : public ActionBase {
	Widget *m_targetWidget;

	ResizeWidgetAction(const int len, Widget *widget);
	virtual void updateTarget(Vec2f sz, float) override;
};

class TickerTape : public StaticText {
	typedef std::deque<TextAction> Actions;

private:
	SizeHint       m_anchor;
	Alignment      m_align;
	Vec2f          m_startOffset;
	Vec2f          m_endOffset;
	TransitionFunc m_transFunc;
	bool           m_alternateOrigin;
	bool           m_overlapTransitions;
	int            m_displayInterval;
	int            m_transitionInterval; 
	int            m_actionCounter;
	Actions        m_actions;

private:
	void setPositions(TextAction &action);
	void startAction(int ndx);

public:
	TickerTape(Container *parent, SizeHint anchor = SizeHint(50), Alignment alignment = Alignment::CENTERED);
	~TickerTape() {
		m_rootWindow->unregisterUpdate(this);
	}

	// add items
	void addItems(const vector<string> &strings);
	void addItems(const string *strings, unsigned n);
	void addItems(const char **strings); // null terminated
	void addItems(const char **strings, unsigned n);

	// start
	void startTicker();

	// configure
	void setOffsets(const Vec2i &startOffset, const Vec2i &endOffset) {
		m_startOffset = Vec2f(startOffset);
		m_endOffset = Vec2f(endOffset);
	}
	void setAnchor(SizeHint a)          { m_anchor = a;             }
	void setAlignment(Alignment a)      { m_align = a;              }
	void setDisplayInterval(int v)      { m_displayInterval = v;    }
	void setTransitionInterval(int v)   { m_transitionInterval = v; }
	void setAlternateOrigin(bool v)     { m_alternateOrigin = v;    }
	void setOverlapTransitions(bool v)  { m_overlapTransitions = v; }
	void setTransitionFunc(TransitionFunc tf) { m_transFunc = tf;   }

	// Widget overrides
	virtual Vec2i getMinSize() const override;
	virtual Vec2i getPrefSize() const override { return getMinSize(); }
	virtual void setSize(const Vec2i &sz) override;
	virtual void render() override;
	virtual void update() override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::TICKER_TAPE); }
	virtual string descType() const override { return "TickerTape"; }
};

}}

#endif
