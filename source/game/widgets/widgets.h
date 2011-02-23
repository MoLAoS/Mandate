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

#include "widgets_base.h"
#include "widget_config.h"

namespace Glest { namespace Widgets {

// =====================================================
// class StaticImage
// =====================================================

class StaticImage : public Widget, public ImageWidget {
public:
	StaticImage(Container* parent);
	StaticImage(Container* parent, Vec2i pos, Vec2i size);
	StaticImage(Container* parent, Vec2i pos, Vec2i size, Texture2D *tex);

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void setStyle() { setWidgetStyle(WidgetType::STATIC_WIDGET); }
	virtual void render();
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

class StaticText : public Widget, public TextWidget {
private:
	bool	m_shadow;
	int		m_shadowOffset;
	bool	m_doubleShadow;
	
public:
	StaticText(Container* parent);
	StaticText(Container* parent, Vec2i pos, Vec2i size);

public:
	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	void setShadow(const Vec4f &colour, int offset=2);
	void setDoubleShadow(const Vec4f &colour1, const Vec4f &colour2, int offset=2);

	virtual void setStyle() { setWidgetStyle(WidgetType::STATIC_WIDGET); }
	virtual void render();
	virtual string desc() { return string("[StaticText: ") + descPosDim() + "]"; }
};

// =====================================================
// class Button
// =====================================================

class Button : public Widget, public TextWidget, public MouseWidget {
protected:
	bool m_doHoverHighlight;

public:
	Button(Container* parent);
	Button(Container* parent, Vec2i pos, Vec2i size, bool hoverHighlight = true);

	virtual void setSize(const Vec2i &sz) override;

	virtual void mouseIn() override { setHover(true); }
	virtual void mouseOut() override { setHover(false); }

	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual void setStyle() { setWidgetStyle(WidgetType::BUTTON); }
	virtual void render() override;
	virtual string desc() override { return string("[Button: ") + descPosDim() + "]"; }

	sigslot::signal<Button*> Clicked;
};

// =====================================================
// class CheckBox
// =====================================================

class CheckBox : public Button, public ImageWidget {
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

	virtual void setStyle() { setWidgetStyle(m_checked ? WidgetType::CHECK_BOX_CHK : WidgetType::CHECK_BOX); }
	//virtual void render() override;
	virtual string desc() override { return string("[CheckBox: ") + descPosDim() + "]"; }
};

// =====================================================
// class TextBox
// =====================================================

class TextBox : public Widget, public MouseWidget, public KeyboardWidget, public TextWidget {
private:
	bool changed;
	string m_inputMask;
	BorderStyle m_normBorderStyle, m_focusBorderStyle;

public:
	TextBox(Container* parent);
	TextBox(Container* parent, Vec2i pos, Vec2i size);

	void setInputMask(const string &allowMask) { m_inputMask = allowMask; }

	void gainFocus();

	virtual void mouseIn()  { setHover(true);  }
	virtual void mouseOut() { setHover(false); }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);

	virtual bool keyDown(Key key);
	virtual bool keyUp(Key key);
	virtual bool keyPress(char c);
	virtual void lostKeyboardFocus();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual void setStyle() { setWidgetStyle(WidgetType::TEXT_BOX); }
	virtual void render();
	virtual string desc() { return string("[TextBox: ") + descPosDim() + "]"; }

	sigslot::signal<TextBox*> TextChanged;
	sigslot::signal<TextBox*> InputEntered;
};

// =====================================================
//  class Slider
// =====================================================

class Slider : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
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

	virtual void setStyle() { setWidgetStyle(WidgetType::SLIDER); }
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
// class CellStrip
// =====================================================

class CellStrip : public Container {
protected:
	Orientation         m_direction;
	vector<WidgetCell*> m_cells;
	SizeHint            m_defualtSizeHint;
	Anchors             m_defaultAnchors;
	bool                m_dirty;

protected:
	void setDirty() { m_dirty = true; }	
	virtual void addChild(Widget* child) override;

public:
	CellStrip(Container *parent, Orientation ld, int cells);
	CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ld, int cells);
	
	void setSizeHint(int i, SizeHint hint) {
		ASSERT_RANGE(i, m_cells.size());
		m_cells[i]->setSizeHint(hint);
	}

	void setPercentageHints(int *hints) {
		for (int i=0; i < m_cells.size(); ++i) {
			m_cells[i]->setSizeHint(SizeHint(hints[i]));
		}
	}

	void layoutCells();

	WidgetCell* getCell(int i) const {
		ASSERT_RANGE(i, m_cells.size());
		return m_cells[i];
	}

	virtual void render() override;
	virtual void setPos(const Vec2i &pos) override;
	virtual void setSize(const Vec2i &sz) override;
	virtual string desc() override { return string("[CellStrip: ") + descPosDim() + "]"; }
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

// =====================================================
// class ToolTip
// =====================================================

class ToolTip : public StaticText {
private:
	void init();

public:
	ToolTip(Container* parent);
	ToolTip(Container* parent, Vec2i pos, Vec2i size);

	virtual void setStyle() { setWidgetStyle(WidgetType::TOOL_TIP); }
	void setText(const string &txt);
};

}}

#endif
