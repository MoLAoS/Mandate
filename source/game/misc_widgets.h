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
#include "tool_tips.h"
#include "hero.h"

namespace Glest { namespace Widgets {
using namespace ProtoTypes;
using Gui::CharacterCreator;

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

// =====================================================
// 	class TraitsDisplay
// =====================================================
typedef vector<Specialization*> ListSpecs;
class TraitsDisplay : public Widget, public MouseWidget, public TextWidget {
private:
    Traits traits;
    ListSpecs specializations;
    CharacterCreator *charCreator;
	FuzzySize m_fuzzySize;
	const FontMetrics *m_fontMetrics;
private:
	void layout();
public:
	TraitsDisplay(Container *parent, Traits traitslist, ListSpecs specs, CharacterCreator *characterCreator);
	FuzzySize getFuzzySize() const          {return m_fuzzySize;}
	void setSize();
	void setFuzzySize(FuzzySize fSize);
	void clear();
	void persist();
	void reset();
	virtual void render() override;
	virtual string descType() const override { return "TraitsDisplay"; }
};

// =====================================================
// 	class SkillsDisplay Helper Stuff
// =====================================================
WRAPPED_ENUM(SkillSection, SKILLS)

struct SkillButton {
	SkillSection	        m_section;
	int				        m_index;

	SkillButton(SkillSection s, int ndx) : m_section(s), m_index(ndx) {}

	SkillButton& operator=(const SkillButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const SkillButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const SkillButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class SkillsDisplay
// =====================================================
class SkillsDisplay : public Widget, public MouseWidget, public TextWidget, public ImageWidget {
private:
    static const int invalidIndex = -1;
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 20;
	static const int skillCellCount = cellWidthCount * cellHeightCount;
	const SkillType *skillTypes[skillCellCount];
	const CommandType *commandTypes[skillCellCount];
    int m_imageSize;

	SkillButton	m_hoverBtn,
                m_pressedBtn;

    CommandTip  *m_toolTip;

    Actions *actions;
    CharacterCreator *charCreator;
	FuzzySize m_fuzzySize;
	const FontMetrics *m_fontMetrics;
private:
	void layout();
public:
	SkillsDisplay(Container *parent, Actions *newActions, CharacterCreator *characterCreator);
	virtual string descType() const override            {return "SkillsDisplay";}
	Actions *getActions()                               {return actions;}

	CommandTip* getCommandTip()                         {return m_toolTip;}
	FuzzySize getFuzzySize() const                      {return m_fuzzySize;}
	void setSkillImage(int i, const Texture2D *image)   {setImage(image, i);}
	void setSkillType(int i, SkillType *st)	            {skillTypes[i] = st;}
	void setCommandType(int i, CommandType *ct)	        {commandTypes[i] = ct;}
	SkillButton getHoverButton() const                  {return m_hoverBtn;}
	SkillButton computeIndex(Vec2i pos, bool screenPos = false);
	void computeSkillPanel();
	void computeSkillInfo(int posDisplay);
	void skillButtonPressed(int posDisplay);

	void setSize();
	void setFuzzySize(FuzzySize fSize);
	void clear();
	void persist();
	void reset();
	virtual void render() override;
	void setToolTipText(const string &hdr, const string &tip, SkillSection i_section);

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual void mouseOut() override;
};

// =====================================================
//  class OptionPanel
// =====================================================
typedef std::pair<Spinner*,Spinner*> SpinnerPair;
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

	ListBoxItem*            addHeading(OptionPanel* headingPnl, const string &txt);
	ListBoxItem*            addLabel(const string &txt);
	CheckBox*               addCheckBox(const string &lbl, bool checked);
	TextBox*                addTextBox(const string &lbl, const string &txt);
	TraitsDisplay*          addTraitsDisplay(Traits traits, ListSpecs specs, CharacterCreator *charCreator);
	SkillsDisplay*          addSkillsDisplay(Actions *actions, CharacterCreator *charCreator);
	DropList*               addDropList(const string &lbl, bool compact = false);
	Spinner*                addSpinner(const string &lbl);
	SpinnerPair             addSpinnerPair(const string &lbl, const string &lbl1, const string &lbl2);

	void setSplitDistance(int v) { m_splitDistance = v; }
	void setScrollPosition(float v) { m_scrollBar->setThumbPos(v); }

	virtual void setSize(const Vec2i &sz) override;

	virtual bool mouseWheel(Vec2i pos, int z) override { m_scrollBar->scrollLine(z > 0); return true; }

	void onScroll(ScrollBar *sb);
	void onHeadingClicked(Widget *cb);
};

}}

#endif
