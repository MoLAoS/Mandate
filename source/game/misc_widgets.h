// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS__MISC_WIDGETS_H_
#define _GLEST_WIDGETS__MISC_WIDGETS_H_

#include "list_widgets.h"

namespace Glest { namespace Widgets {

class OptionContainer : public Container {
private:
	StaticText* m_label;
	Widget*		m_widget;

	bool	m_abosulteLabelSize;
	int		m_labelSize;

public:
	OptionContainer(Container* parent, Vec2i pos, Vec2i size, const string &labelText);

	void setLabelWidth(int value, bool absolute);

	///@todo deprecate, override addChild, assume second child is contained widget
	void setWidget(Widget* widget);
	Widget* getWidget() { return m_widget; }

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual string descType() const override { return "OptionBox"; }
};

WRAPPED_ENUM( ScrollAction, TOP, MAINTAIN, BOTTOM );

class ScrollText : public CellStrip, public TextWidget, public sigslot::has_slots {
protected:
	ScrollBar  *m_scrollBar;
	StaticText *m_staticText;
	string      m_origString;
	Anchors     m_anchorNoScroll;
	Anchors     m_anchorWithScroll;

private:
	void init();
	void setAndWrapText(const string &txt);

public:
	ScrollText(Container* parent);
	ScrollText(Container* parent, Vec2i pos, Vec2i size);

	void recalc();
	void onScroll(ScrollBar*);
	void setText(const string &txt, ScrollAction scroll = ScrollAction::TOP);

	virtual void setSize(const Vec2i &sz) override;
	virtual void render() override;
};


// =====================================================
// class InputBox
// =====================================================

class InputBox : public TextBox {
public:
	InputBox(Container *parent);
//	InputBox(Container *parent, Vec2i pos, Vec2i size);

	virtual bool keyDown(Key key) override;
	sigslot::signal<Widget*> Escaped;
	virtual string descType() const override { return "InputBox"; }
};

// =====================================================
// 	class SpinnerValueBox
// =====================================================

class SpinnerValueBox : public StaticText {
public:
	SpinnerValueBox(Container *parent) : StaticText(parent) {
		setWidgetStyle(WidgetType::TEXT_BOX);
	}
	virtual void setStyle() override { setWidgetStyle(WidgetType::TEXT_BOX); }
};

// =====================================================
// 	class Spinner
// =====================================================

class Spinner : public CellStrip,  public sigslot::has_slots {
private:
	SpinnerValueBox  *m_valueBox;
	ScrollBarButton  *m_upButton;
	ScrollBarButton  *m_downButton;
	int               m_minValue;
	int               m_maxValue;
	int               m_increment;
	int               m_value;

	void onButtonFired(Widget *btn);

public:
	Spinner(Container *parent);

	void setRanges(int min, int max) { m_minValue = min; m_maxValue = max; }
	void setIncrement(int inc) { m_increment = inc; }
	void setValue(int val) { 
		m_value = clamp(val, m_minValue, m_maxValue);
		m_valueBox->setText(intToStr(m_value));
	}

	int getValue() const { return m_value; }

	sigslot::signal<Widget*> ValueChanged;
};

// =====================================================
// 	class CheckBoxHolder
// =====================================================

class CheckBoxHolder : public CellStrip {
public:
	CheckBoxHolder(Container *parent)
			: CellStrip(parent, Orientation::HORIZONTAL, 3) {
		setSizeHint(0, SizeHint());
		setSizeHint(1, SizeHint(-1, g_widgetConfig.getDefaultItemHeight()));
		setSizeHint(2, SizeHint());
		setAnchors(Anchors::getFillAnchors());
	}
};

// =====================================================
// 	class RelatedDoubleOption
// =====================================================

class RelatedDoubleOption : public CellStrip {
public:
	RelatedDoubleOption(Container *parent, const string &title, const string &txt1, const string &txt2)
			: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 5) {
		setSizeHint(0, SizeHint(40));
		setSizeHint(1, SizeHint(10));
		setSizeHint(2, SizeHint(20));
		setSizeHint(3, SizeHint(10));
		setSizeHint(4, SizeHint(20));

		Anchors dwAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2),
			Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2));
		setAnchors(dwAnchors);

		StaticText *label = new StaticText(this);
		label->setText(title);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(0);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);

		label = new StaticText(this);
		label->setText(txt1);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(1);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);

		label = new StaticText(this);
		label->setText(txt2);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(3);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		label->setAlignment(Alignment::FLUSH_RIGHT);
		label->borderStyle().setSizes(0, 0, 10, 0);
	}

	void setCustomSplits(int label, int val1, int val2) {
		setSizeHint(1, SizeHint(label));
		setSizeHint(2, SizeHint(val1));
		setSizeHint(3, SizeHint(label));
		setSizeHint(4, SizeHint(val2));
	}
};

typedef std::pair<Spinner*,Spinner*> SpinnerPair;

// =====================================================
//  class OptionPanel
// =====================================================

class OptionPanel : public CellStrip, public MouseWidget, public sigslot::has_slots {
private:
	CellStrip *m_list;
	ScrollBar *m_scrollBar;

	vector<Vec2i> m_origPositions;
	int           m_scrollOffset;
	int           m_splitDistance;

	std::map<std::string, Widget *> m_headings;

	SizeHint m_scrollSizeHint;
	SizeHint m_noScrollSizeHint;

public:
	OptionPanel(CellStrip *parent, int cell);

	ListBoxItem* addHeading(OptionPanel* headingPnl, const string &txt);
	ListBoxItem* addLabel(const string &txt);
	CheckBox*   addCheckBox(const string &lbl, bool checked);
	TextBox*    addTextBox(const string &lbl, const string &txt);
	DropList*   addDropList(const string &lbl, bool compact = false);
	Spinner*    addSpinner(const string &lbl);
	SpinnerPair addSpinnerPair(const string &lbl, const string &lbl1, const string &lbl2);

	void setSplitDistance(int v) { m_splitDistance = v; }
	void setScrollPosition(float v) { m_scrollBar->setThumbPos(v); }

	virtual void setSize(const Vec2i &sz) override;

	virtual bool mouseWheel(Vec2i pos, int z) override { m_scrollBar->scrollLine(z > 0); return true; }

	void onScroll(ScrollBar *sb);
	void onHeadingClicked(Widget *cb);
};

}}

#endif
