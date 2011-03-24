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
// class StaticImage
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
	void setDefaultImage(int ndx = 0);
	void setActive(int ndx = 0);

	// Widget overrides
	virtual string descType() const override { return "ImageSet"; }
	virtual void render() override { renderImage(m_active); }
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
// class Button
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
	sigslot::signal<TextBox*> TextChanged;
	sigslot::signal<TextBox*> InputEntered;
};

// =====================================================
// class CellStrip
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

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual void render() override;
	virtual string descType() const override { return "Panel"; }
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

	virtual Vec2i getPrefSize() const override;
	virtual Vec2i getMinSize() const override;

	virtual void render() override {
		renderImage();
		Container::render();
	}

	virtual string descType() const override { return "PicturePanel"; }
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

	void setText(const string &txt);

	// Widget overrides
	virtual void setStyle() override { setWidgetStyle(WidgetType::TOOL_TIP); }
	virtual string descType() const override { return "Tooltip"; }
};

class OptionWidget : public CellStrip {
public:
	OptionWidget(Container *parent, const string &text)
			: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 2) {
		setSizeHint(0, SizeHint(40));
		setSizeHint(1, SizeHint(60));
		Anchors dwAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2),
			Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2));
		setAnchors(dwAnchors);
		StaticText *label = new StaticText(this);
		label->setText(text);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(0);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
		
	}
};

class DoubleOption : public CellStrip {
public:
	DoubleOption(Container *parent, const string &txt1, const string &txt2)
			: CellStrip(parent, Orientation::HORIZONTAL, Origin::FROM_LEFT, 4) {
		setSizeHint(0, SizeHint(40));
		setSizeHint(1, SizeHint(10));
		setSizeHint(2, SizeHint(40));
		setSizeHint(3, SizeHint(10));
		Anchors dwAnchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2),
			Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 2));
		setAnchors(dwAnchors);
		StaticText *label = new StaticText(this);
		label->setText(txt1);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(0);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));

		label = new StaticText(this);
		label->setText(txt2);
		label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		label->setCell(2);
		label->setAnchors(Anchor(AnchorType::RIGID, 0));
	}

	void setCustomSplit(bool first, int label) {
		assert(label > 0 && label < 50);
		int content = 50 - label;
		int i = first ? 0 : 2;
		setSizeHint(i++, SizeHint(label));
		setSizeHint(i, SizeHint(content));
	}
};


}}

#endif
