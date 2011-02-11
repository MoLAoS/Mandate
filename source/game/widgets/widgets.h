// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_INCLUDED_
#define _GLEST_WIDGETS_INCLUDED_

#include <stack>

#include "sigslot.h"

#include "widgets_base.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

// =====================================================
// class StaticImage
// =====================================================

class StaticImage : public CellWidget, public ImageWidget {
public:
	StaticImage(Container* parent)
			: CellWidget(parent)
			, ImageWidget(this) {}

	StaticImage(Container* parent, Vec2i pos, Vec2i size) 
			: CellWidget(parent, pos, size)
			, ImageWidget(this) {}

	StaticImage(Container* parent, Vec2i pos, Vec2i size, Texture2D *tex)
			: CellWidget(parent, pos, size)
			, ImageWidget(this, tex) {}

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render() { renderImage(); }
	virtual string desc() { return string("[StaticImage: ") + descPosDim() + "]"; }
};

// =====================================================
//	class Imageset
//
/// Images are extracted from a single source image
// =====================================================

class Imageset : public StaticImage {
private:
	int m_active;
	int m_defaultImage;
public:
	Imageset(Container* parent) 
			: StaticImage(parent)
			, m_active(0)
			, m_defaultImage(0) {
	}

	/// Constructor for uniform image sizes, see addImages
	Imageset(Container* parent, const Texture2D *source, int width, int height)
			: StaticImage(parent)
			, m_active(0)
			, m_defaultImage(0) {
		addImages(source, width, height);
	}

	// non-uniform squares
	/*Imageset(Texture2D, squarespec) {
		use the squarespec to extract the images
	}*/
	void addImages(const Texture2D *source, int width, int height);
	virtual void render() { renderImage(m_active); }

	void setDefaultImage(int ndx = 0);
	void setActive(int ndx = 0);
};

// =====================================================
//	class Animset
/// 2D animation control for imagesets
// =====================================================

class Animset : public Widget {
private:
	Imageset *m_imageset;
	int m_currentFrame;
	int m_fps;
	float m_timeElapsed;
	int m_start, m_end;
	bool m_loop;

public:
	Animset(Container* parent, Imageset *imageset, int fps) 
			: Widget(parent)
			, m_imageset(imageset)
			, m_currentFrame(0)
			, m_fps(fps)
			, m_timeElapsed(0.0)
			, m_start(0)
			, m_loop(true) {
		m_end = m_imageset->getNumImages()-1;
	}

	/// rendering is handled by Imageset
	virtual void render() {}
	virtual void update();
	virtual string desc() { return string("[Animset: ") + "]"; }

	void setRange(int start, int end) { m_start = start; m_end = end; }
	const Texture2D *getCurrent() { return m_imageset->getImage(m_currentFrame); }
	void setFps(int v) { m_fps = v; }
	void play() { setEnabled(true); }
	void stop() { setEnabled(false); }
	void reset() { m_currentFrame = m_start; }
	void loop(bool v) { m_loop = v; }
};

// =====================================================
// class StaticText
// =====================================================

class StaticText : public CellWidget, public TextWidget {
private:
	bool	m_shadow;
	int		m_shadowOffset;
	bool	m_doubleShadow;
	
public:
	StaticText(Container* parent)
			: CellWidget(parent) , TextWidget(this)
			, m_shadow(false), m_doubleShadow(false), m_shadowOffset(2) {
		m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	}

	StaticText(Container* parent, Vec2i pos, Vec2i size)
			: CellWidget(parent, pos, size), TextWidget(this)
			, m_shadow(false), m_doubleShadow(false), m_shadowOffset(2) {
		m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	}

public:
	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	void setShadow(const Vec4f &colour, int offset=2);
	void setDoubleShadow(const Vec4f &colour1, const Vec4f &colour2, int offset=2);

	virtual void render();
	virtual string desc() { return string("[StaticText: ") + descPosDim() + "]"; }
};

// =====================================================
// class Button
// =====================================================

class Button : public CellWidget, public TextWidget, public ImageWidget, public MouseWidget {
protected:
	bool m_hover;
	bool m_pressed;
	bool m_doHoverHighlight;
	bool m_defaultTexture;

public:
	Button(Container* parent);
	Button(Container* parent, Vec2i pos, Vec2i size, bool defaultTex = true, bool hoverHighlight = true);

	virtual void setSize(const Vec2i &sz) override;

	virtual void mouseIn() override { m_hover = true; }
	virtual void mouseOut() override { m_hover = false; }

	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual void render() override;
	virtual string desc() override { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<Button*> Clicked;
};

// =====================================================
// class CheckBox
// =====================================================

class CheckBox : public Button {
protected:
	bool m_checked;

public:
	CheckBox(Container* parent);
	CheckBox(Container* parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz) override;
	void setChecked(bool v) { m_checked = v; }
	bool isChecked() const { return m_checked; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual void render() override;
	virtual string desc() override { return string("[CheckBox: ") + descPosDim() + "]"; }
};

// =====================================================
// class TextBox
// =====================================================

class TextBox : public CellWidget, public MouseWidget, public KeyboardWidget, public TextWidget {
private:
	bool hover;
	bool focus;
	bool changed;
	string m_inputMask;
	BorderStyle m_normBorderStyle, m_focusBorderStyle;

public:
	TextBox(Container* parent);
	TextBox(Container* parent, Vec2i pos, Vec2i size);

	void setInputMask(const string &allowMask) { m_inputMask = allowMask; }

	void gainFocus();

	virtual void mouseIn()	{ hover = true;	 }
	virtual void mouseOut() { hover = false;	}

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual bool keyDown(Key key);
	virtual bool keyUp(Key key);
	virtual bool keyPress(char c);
	virtual void lostKeyboardFocus();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[TextBox: ") + descPosDim() + "]"; }

	sigslot::signal<TextBox*> TextChanged;
	sigslot::signal<TextBox*> InputEntered;
};

// =====================================================
//  class Slider
// =====================================================

class Slider : public CellWidget, public MouseWidget, public ImageWidget, public TextWidget {
private:
	float	m_sliderValue;
	bool	m_thumbHover,
			m_thumbPressed,
			m_shaftHover;
	int		m_shaftOffset, 
			m_shaftSize,
			m_thumbCentre,
			m_valSize;
	Vec2i	m_thumbPos,
			m_thumbSize;
	Vec2i	m_titlePos,
			m_valuePos;
	string	m_title;

	BorderStyle m_shaftStyle;

	void recalc();

public:
	Slider(Container* parent, Vec2i pos, Vec2i size, const string &title);

	void setValue(float val) { m_sliderValue = val; recalc(); }
	float getValue() const { return m_sliderValue; }

	void setSize(const Vec2i &size) {
		Widget::setSize(size);
		recalc();
	}

	virtual void mouseOut();

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);

	void render();

	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(300,32); }
	virtual string desc() { return string("[Slider: ") + descPosDim() + "]"; }

	sigslot::signal<Slider*> ValueChanged;
};

// =====================================================
//  class VerticalScrollBar
// =====================================================

class VerticalScrollBar : public CellWidget, public ImageWidget, public MouseWidget {
private:
	WRAPPED_ENUM( Part, NONE, UP_BUTTON, DOWN_BUTTON, THUMB, UPPER_SHAFT, LOWER_SHAFT );

private:
	Part hoverPart, pressedPart;
	// special case conditions? thumb fills shaft and thumb is so small we need to render it bigger.
	//bool fullThumb, smallThumb;
	int shaftOffset, shaftHeight;
	int thumbOffset, thumbSize;
	int totalRange, availRange, lineSize;
	int timeCounter;
	bool moveOnMouseUp;
	int topOffset;

	BorderStyle m_shaftStyle;
	BorderStyle m_thumbStyle;

	void init();
	void recalc();
	Part partAt(const Vec2i &pos);

public:
	VerticalScrollBar(Container* parent);
	VerticalScrollBar(Container* parent, Vec2i pos, Vec2i size);
	~VerticalScrollBar();

	void setRanges(int total, int avail, int line = 60);
	void setTotalRange(int max) { totalRange = max; }
	void setActualRane(int avail) { availRange = avail; recalc(); }
	void setLineSize(int line) { lineSize = line; }

	int getRangeOffset() const { return int(thumbOffset / float(shaftHeight) * totalRange); }
	void setOffset(float percent);

	void scrollLine(bool i_up);

//	int getTotalRange() const { return totalRange; }
//	int getActualRange() const { return actualRange; }

	virtual void setSize(const Vec2i &sz) { Widget::setSize(sz); recalc(); }

	virtual void update();
	virtual void mouseOut() { hoverPart = Part::NONE; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[VerticalScrollBar: ") + descPosDim() + "]"; }

	sigslot::signal<VerticalScrollBar*> ThumbMoved;
};

// =====================================================
// class WidgetStrip
// =====================================================

class WidgetStrip : public Container {
private:
	Orientation	 m_direction;
//	Origin	 m_origin;
	int              m_childSlotCount;
	SizeHint		 m_defualtSizeHint;
	Anchors          m_defaultAnchors;
	bool             m_dirty;

private:
	void setDirty() { m_dirty = true; }	

public:
	WidgetStrip(Container *parent, Orientation ld);
	WidgetStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ld);

	void layoutCells();

	void setDefaultSizeHint(SizeHint sizeHint) { m_defualtSizeHint = sizeHint; }
	void setDefaultAnchors(Anchors anchors) { m_defaultAnchors = anchors; }

	virtual void addChild(CellWidget* child);
	virtual void addChild(Widget* child) override;
	virtual void setCellRect(const Vec2i &pos, const Vec2i &size) override;

	virtual void render() override;
	virtual void setPos(const Vec2i &pos) override;
	virtual void setSize(const Vec2i &sz) override;
	virtual string desc() override { return string("[WidgetStrip: ") + descPosDim() + "]"; }
};

// =====================================================
// class Panel
// =====================================================

class Panel : public Container {
protected:
	int		widgetPadding;	// padding between child widgets
	bool	autoLayout;
	Orientation	layoutDirection;
	Origin	layoutOrigin;

	Panel(WidgetWindow* window);
	void layoutVertical();
	void layoutHorizontal();

public:
	void setLayoutParams(bool autoLayout, Orientation dir, Origin origin = Origin::CENTRE);
	
public:
	Panel(Container* parent);
	Panel(Container* parent, Vec2i pos, Vec2i size);

	void setAutoLayout(bool val) { autoLayout = val; }
	void setPaddingParams(int panelPad, int widgetPad);

	virtual void addChild(Widget* child) override;
	virtual void remChild(Widget* child) override { Container::remChild(child); }
	virtual void delChild(Widget* child) override { Container::delChild(child); }
	virtual void clear() override { Container::clear(); }

	void setLayoutOrigin(Origin lo) { layoutOrigin = lo; }
	virtual void layoutChildren();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[Panel: ") + descPosDim() + "]"; }
};

// =====================================================
// class PicturePanel
// =====================================================

class PicturePanel : public Panel, public ImageWidget {
public:
	PicturePanel(Container* parent)
			: Panel(parent)
			, ImageWidget(this) {
	}

	PicturePanel(Container* parent, Vec2i pos, Vec2i size) 
			: Panel(parent, pos, size)
			, ImageWidget(this) {
	}

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render() {
		renderImage();
		Container::render();
	}

	virtual string desc() { return string("[PicturePanel: ") + descPosDim() + "]"; }
};

class ListBoxItem;

// =====================================================
// class ListBase
// =====================================================

class ListBase : public Panel {
protected:
	ListBoxItem* selectedItem;
	Font *itemFont;
	int selectedIndex;
	vector<string> listItems;

	ListBase(WidgetWindow* window);

public:
	ListBase(Container* parent);
	ListBase(Container* parent, Vec2i pos, Vec2i size);

	virtual void addItems(const vector<string> &items) = 0;
	virtual void addItem(const string &item) = 0;
//	virtual void clearItems() = 0;

	virtual void setSelected(int index) = 0;

	int getSelectedIndex() { return selectedIndex; }
	ListBoxItem* getSelectedItem() { return selectedItem; }

	unsigned getItemCount() const { return listItems.size(); }


	sigslot::signal<ListBase*> SelectionChanged;
	sigslot::signal<ListBase*> SameSelected;
};

// =====================================================
// class ListBox
// =====================================================

class ListBox : public ListBase, public MouseWidget, public sigslot::has_slots {
public:
	//WRAPPED_ENUM( ScrollSetting, NEVER, AUTO, ALWAYS );
private:
	vector<int> yPositions; // 'original' (non-scrolled) y coords of children (sans scrollBar)

protected:
	vector<ListBoxItem*> listBoxItems;

	VerticalScrollBar* scrollBar;
	//ScrollSetting scrollSetting;
	int scrollWidth;

public:
	ListBox(Container* parent);
	ListBox(Container* parent, Vec2i pos, Vec2i size);
	ListBox(WidgetWindow* window);

	virtual void addItems(const vector<string> &items);
	virtual void addItem(const string &item);
	void addColours(const vector<Vec3f> &colours);
//	virtual void clearItems();

	virtual void setSelected(int index);
	//virtual void setSelected(ListBoxItem *item);
	void onSelected(ListBoxItem* item);
	
	virtual void setSize(const Vec2i &sz);
	void setScrollBarWidth(int width);

	virtual void layoutChildren();
//	void setScrollSetting(ScrollSetting setting);

	bool mouseWheel(Vec2i pos, int z);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	int getPrefHeight(int childCount = -1);

	virtual string desc() { return string("[ListBox: ") + descPosDim() + "]"; }

	void onScroll(VerticalScrollBar*);

	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

// =====================================================
// class ListBoxItem
// =====================================================

class ListBoxItem : public StaticText, public MouseWidget {
private:
	bool selected;
	bool hover;
	bool pressed;

protected:

public:
	ListBoxItem(ListBase* parent);
	ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz);
	ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour);

	void setSelected(bool s) { selected = s; }
	void setBackgroundColour(const Vec3f &colour);

	virtual void mouseIn();
	virtual void mouseOut();
	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void render();
	virtual string desc() { return string("[ListBoxItem: ") + descPosDim() + "]"; }

	sigslot::signal<ListBoxItem*> Selected;
	sigslot::signal<ListBoxItem*> Clicked;
	// inherited signals:
	//		Widget::Destoyed
};

// =====================================================
// class DropList
// =====================================================

class DropList : public ListBase, public MouseWidget, public sigslot::has_slots {
private:
	ListBox* floatingList;
	Button* button;
	int dropBoxHeight;

	void expandList();
	void layout();

public:
	DropList(Container* parent);
	DropList(Container* parent, Vec2i pos, Vec2i size);

	virtual void setSize(const Vec2i &sz);
	void setDropBoxHeight(int h) { dropBoxHeight = h; }

	void addItems(const vector<string> &items);
	void addItem(const string &item);
	void clearItems();

	void setSelected(int index);
	void setSelected(const string &item);

	bool mouseWheel(Vec2i pos, int z);

	// event handlers
	void onBoxClicked(ListBoxItem*);
	void onExpandList(Button*);
	void onSelectionMade(ListBase*);
	void onSameSelected(ListBase*);
	void onListDisposed(Widget*);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;
	//virtual void setEnabled(bool v) {
	//}

	virtual string desc() { return string("[DropList: ") + descPosDim() + "]"; }

	sigslot::signal<DropList*> ListExpanded;
	sigslot::signal<DropList*> ListCollapsed;
	// inherited signals:
	//		ListBase::SelectionChanged
	//		Widget::Destoyed
};

class ToolTip : public StaticText {
private:
	void init();

public:
	ToolTip(Container* parent);
	ToolTip(Container* parent, Vec2i pos, Vec2i size);

	void setText(const string &txt);
};

}}

#endif
