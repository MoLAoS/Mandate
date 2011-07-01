// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
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
//  class StaticImage
// =====================================================

class StaticImage : public Widget, public ImageWidget {
public:
	StaticImage(Container* parent);
	StaticImage(Container* parent, Vec2i pos, Vec2i size);
	StaticImage(Container* parent, Vec2i pos, Vec2i size, Texture2D *tex);

	// Widget overrides
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::STATIC_WIDGET); }
	virtual void render() override;
	virtual string descType() const override { return "StaticImage"; }
};

// =====================================================
//	class Imageset
// =====================================================

/** Images are extracted from a single source image */
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
	void setDefaultImage(int ndx = 0);
	void setActive(int ndx = 0);

	// Widget overrides
	virtual string descType() const override { return "ImageSet"; }
	virtual void render() override { renderImage(m_active); }
};

// =====================================================
//	class Animset
// =====================================================

/** 2D animation control for imagesets */
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

	void setRange(int start, int end) { m_start = start; m_end = end; }
	const Texture2D *getCurrent() { return m_imageset->getImage(m_currentFrame); }
	void setFps(int v) { m_fps = v; }
	void play() { setEnabled(true); }
	void stop() { setEnabled(false); }
	void reset() { m_currentFrame = m_start; }
	void loop(bool v) { m_loop = v; }

	// Widget overrides
	/// rendering is handled by Imageset
	virtual void render() override {}
	virtual void update() override;
	virtual string descType() const override { return "Animset"; }
};

// =====================================================
//  class StaticText
// =====================================================

class StaticText : public Widget, public TextWidget {
private:
	bool   m_shadow;
	bool   m_doubleShadow;
	
public:
	StaticText(Container* parent);
	StaticText(Container* parent, Vec2i pos, Vec2i size);

public:
	void setShadow(const Vec4f &colour, int offset=2);
	void setDoubleShadow(const Vec4f &colour1, const Vec4f &colour2, int offset=2);

	// Widget overrides
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::STATIC_WIDGET); }
	virtual void render() override;
	virtual string descType() const override { return "StaticText"; }
};

// =====================================================
//  class Button
// =====================================================

class Button : public Widget, public TextWidget, public MouseWidget {
protected:
	bool m_doHoverHighlight;

public:
	Button(Container* parent);
	Button(Container* parent, Vec2i pos, Vec2i size, bool hoverHighlight = true);

	// MouseWidget overrides
	virtual void mouseIn() override { setHover(true); }
	virtual void mouseOut() override { setHover(false); }
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	// Widget overrides
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual void setSize(const Vec2i &sz) override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::BUTTON); }
	virtual void render() override;
	virtual string descType() const override { return "Button"; }

	// Signals
	sigslot::signal<Widget*> Clicked;
};

// =====================================================
//  class CheckBox
// =====================================================

class CheckBox : public Button, public ImageWidget {
protected:
	bool m_checked;

public:
	CheckBox(Container* parent);
	CheckBox(Container* parent, Vec2i pos, Vec2i size);

	void setChecked(bool v) { m_checked = v; setStyle(); }
	bool isChecked() const { return m_checked; }

	// MouseWidget overrides
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	// Widget overrides
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual void setSize(const Vec2i &sz) override;
	virtual void setStyle() override { setWidgetStyle(m_checked ? WidgetType::CHECK_BOX_CHK : WidgetType::CHECK_BOX); }
	virtual string descType() const override { return "CheckBox"; }
};

// =====================================================
//  class TextBox
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

	// MouseWidget overrides
	virtual void mouseIn() override { setHover(true);  }
	virtual void mouseOut() override { setHover(false); }
	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;

	// KeyboardWidget overrides
	virtual bool keyDown(Key key) override;
	virtual bool keyUp(Key key) override;
	virtual bool keyPress(char c) override;
	virtual void lostKeyboardFocus() override;

	// Widget overrides
	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;
	virtual void setStyle() override { setWidgetStyle(WidgetType::TEXT_BOX); }
	virtual void render() override;
	virtual string descType() const override { return "TextBox"; }

	// Signals
	sigslot::signal<Widget*> TextChanged;
	sigslot::signal<Widget*> InputEntered;
};

// =====================================================
//  class CellStrip
// =====================================================

class CellStrip : public Container {
protected:
	typedef vector<CellInfo>    CellInfos;

protected:
	Orientation         m_orientation;
	Origin              m_origin; // layout from
	//vector<WidgetCell*> m_cells;
	SizeHint            m_defualtSizeHint;
	Anchors             m_defaultAnchors;
	bool                m_dirty;
	CellInfos           m_cells2;

protected:
	CellStrip(WidgetWindow *window, Orientation ortn, Origin orgn, int cells);

public:
	CellStrip(Container *parent, Orientation ortn);
	CellStrip(Container *parent, Orientation ortn, int cells);
	CellStrip(Container *parent, Orientation ortn, Origin orgn, int cells);
	CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ortn);
	CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ortn, Origin orgn, int cells);

protected:
//	void setCustumCell(int ndx, WidgetCell *cell);
	void setDirty() { m_dirty = true; }	
	
	// Container override
	virtual void addChild(Widget* child) override;

	void render(bool clip);

public:
	void setSizeHint(int i, SizeHint hint) {
		ASSERT_RANGE(i, m_cells2.size());
		//m_cells[i]->setSizeHint(hint);
		m_cells2[i].m_hint = hint;
	}

	void setPercentageHints(int *hints) {
		for (int i=0; i < m_cells2.size(); ++i) {
			//m_cells[i]->setSizeHint(SizeHint(hints[i]));
			m_cells2[i].m_hint = SizeHint(hints[i]);
		}
	}

	virtual void clear() override {
		Container::clear();
		m_cells2.clear();
	}

	virtual Rect2i getCellArea(int cell) const override {
		return Rect2i(m_cells2[cell].m_pos, m_cells2[cell].m_pos + m_cells2[cell].m_size);
	}

	virtual SizeHint getSizeHint(int cell) const override {
		return m_cells2[cell].m_hint;
	}

	virtual void layoutCells();

	//WidgetCell* getCell(int i) const {
	//	ASSERT_RANGE(i, m_cells.size());
	//	return m_cells[i];
	//}

	//void clearCells();
	//void deleteCells();
	void addCells(int n);
	int  getCellCount() const { return m_cells2.size(); }

	// Widget overrides
	virtual void render() override;
	virtual void setPos(const Vec2i &pos) override;
	virtual void setSize(const Vec2i &sz) override;
	virtual string descType() const override { return "CellStrip"; }
};

// =====================================================
//  class PicturePanel
// =====================================================

class PicturePanel : public Container, public ImageWidget {
public:
	PicturePanel(Container* parent)
			: Container(parent)
			, ImageWidget(this) {
	}

	PicturePanel(Container* parent, Vec2i pos, Vec2i size) 
			: Container(parent, pos, size)
			, ImageWidget(this) {
	}

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual void render() override {
		renderImage();
		Container::render();
	}

	virtual string descType() const override { return "PicturePanel"; }
};

// =====================================================
//  class OptionWidget
// =====================================================

/** Simple container coupling (horizontally) a label (StaticText) with a user supplied 'option' widget.
  * Splits space 40/60 (label/option) use setPercentSplit() or setAbsoluteSplt() to chnage.
  */
class OptionWidget : public CellStrip {
public:
	OptionWidget(Container *parent, const string &text);
	/** set the option widget
	  * @param widget the 'option widget' (created as a child of 'this' widget!) */
	void setOptionWidget(Widget *widget);
	/** set a Custom split for the label/option pair
	  * @param lblPercent percentage of space to assign the label (<100, option widget get remainder) */
	void setPercentSplit(int lblPercent);	
	/** set a Custom split for the label/option pair
	  * @param val absolute pixel size for label/option
	  * @param label true if val is the size for the label, false for the option widget */
	void setAbsoluteSplit(int val, bool label);

	StaticText* getLabel() { return static_cast<StaticText*>(m_children[0]); }
};

// =====================================================
//  class DoubleOption
// =====================================================

/** Container for coupling (horizontally) two labels (StaticText) with user supplied 'option' widgets,
  * Each pair is split 40/10 (label/option) use setCustomSplit() to change.
  */
class DoubleOption : public CellStrip {
public:
	DoubleOption(Container *parent, const string &txt1, const string &txt2);
	/** set one of the option widgets
	  * @param first true if this is the first option widget, false for the second
	  * @param widget the 'option widget' (created as a child of 'this' widget!) */
	void setOptionWidget(bool first, Widget *widget);
	/** set a Custom split for one of the label/option pairs
	  * @param first true to set split for first label/option pair, false for second
	  * @param label percentage of _total_ space to assign the label (<50, option widget get remainder) */
	void setCustomSplit(bool first, int label);
};

// =====================================================
//  class TabWidget
// =====================================================

/** A widget that has a cell for buttons and an associated cell that appears when
  * the button is pressed.
  */
class TabWidget : public CellStrip, public sigslot::has_slots {
protected:
	// Item order is important.
	// Pointers owned elsewhere, don't delete here.
	typedef vector<Button*> Buttons;
	typedef vector<CellStrip*> Pages;
private:
	Buttons m_buttons;
	Pages m_pages;
	int m_active;
	CellStrip *m_btnPnl;
	Anchors m_anchors;

protected:
	void onButtonClicked(Widget *widget);
	int getButtonPos(Button *button);
	Button* createButton(const string &text);

public:
	TabWidget(Container *parent);

	void add(const string &text, CellStrip *cellStrip);

	int getActivePage() const { return m_active; }
	void setActivePage(int i);
};


}}

#endif
