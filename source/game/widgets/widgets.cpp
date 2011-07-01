// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "widgets.h"

#include <sstream>

#include "metrics.h"
#include "util.h"
#include "renderer.h"
#include "texture_gl.h"
#include "game_constants.h"
//#include "core_data.h"

#include "widget_window.h"

#include "leak_dumper.h"

using Shared::Util::deleteValues;
using Shared::Graphics::Texture;
using namespace Shared::Graphics::Gl;
using Glest::Graphics::Renderer;
//using namespace Glest::Global;

namespace Glest { namespace Widgets {

// =====================================================
//  class StaticText
// =====================================================

StaticText::StaticText(Container* parent)
		: Widget(parent) , TextWidget(this)
		, m_shadow(false), m_doubleShadow(false) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

StaticText::StaticText(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size), TextWidget(this)
		, m_shadow(false), m_doubleShadow(false) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

void StaticText::render() {
	if (!isVisible()) {
		return;
	}
	Widget::renderBackground();
	if (TextWidget::getText(0) != "") {
		if (m_doubleShadow) {
			TextWidget::renderTextDoubleShadowed();
		} else if (m_shadow) {
			TextWidget::renderTextShadowed();
		} else {
			TextWidget::renderText();
		}
	}
	Widget::renderForeground();
}

Vec2i StaticText::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra = getBordersAll();
	return txtDim + xtra;
}

Vec2i StaticText::getPrefSize() const {
	return getMinSize();
}

void StaticText::setShadow(const Vec4f &colour, int offset) {
	m_shadow = true;
	TextWidget::setTextShadowColour(colour);
	TextWidget::setTextShadowOffset(Vec2i(offset));
}

void StaticText::setDoubleShadow(const Vec4f &colour1, const Vec4f &colour2, int offset) {
	m_doubleShadow = true;
	TextWidget::setTextShadowColours(colour1, colour2);
	TextWidget::setTextShadowOffset(Vec2i(offset));
}

// =====================================================
//  class StaticImage
// =====================================================

StaticImage::StaticImage(Container* parent)
		: Widget(parent)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

StaticImage::StaticImage(Container* parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

StaticImage::StaticImage(Container* parent, Vec2i pos, Vec2i size, Texture2D *tex)
		: Widget(parent, pos, size)
		, ImageWidget(this, tex) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

Vec2i StaticImage::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = getBordersAll();
	return imgDim + xtra;
}

Vec2i StaticImage::getPrefSize() const {
	return getMinSize();
}

void StaticImage::render() {
	if (!isVisible()) {
		return;
	}
	Widget::renderBackground();
	ImageWidget::renderImage();
	Widget::renderForeground();
}

// =====================================================
//  class Button
// =====================================================

Button::Button(Container* parent)
		: Widget(parent)
		, TextWidget(this)
		, MouseWidget(this)
		, m_doHoverHighlight(true) {
	setWidgetStyle(WidgetType::BUTTON);
}

Button::Button(Container* parent, Vec2i pos, Vec2i size, bool hoverHighlight)
		: Widget(parent, pos, size)
		, TextWidget(this)
		, MouseWidget(this)
		, m_doHoverHighlight(hoverHighlight) {
	setWidgetStyle(WidgetType::BUTTON);
}

Vec2i Button::getPrefSize() const {
	Vec2i imgSize(0);
	if (m_borderStyle.m_type == BorderType::TEXTURE) {
		const Texture2D *tex = m_rootWindow->getConfig()->getTexture(m_borderStyle.m_imageNdx);
		imgSize = Vec2i(tex->getPixmap()->getW(), tex->getPixmap()->getH());
	}
	Vec2i txtSize(getMinSize());
	Vec2i res(std::max(imgSize.x, txtSize.x), std::max(imgSize.y, txtSize.y));
	return res;
}

Vec2i Button::getMinSize() const {
	Vec2i res = TextWidget::getTextDimensions() + getBordersAll();
	res.x = std::min(res.x, 16);
	res.y = std::min(res.y, 16);
	return res;
}

void Button::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
}

bool Button::mouseMove(Vec2i pos) {
	//WIDGET_LOG( descLong() << " : Button::mouseMove( " << pos << " )");
	if (isEnabled()) {
		if (isHovered() && !isInside(pos)) {
			mouseOut();
		}
		if (!isHovered() && isInside(pos)) {
			mouseIn();
		}
	}
	return true;
}

bool Button::mouseDown(MouseButton btn, Vec2i pos) {
	//WIDGET_LOG( descLong() << " : Button::mouseDown( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		setFocus(true);
	}
	return true;
}

bool Button::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( descLong() << " : Button::mouseUp( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (isFocused() && isHovered()) {
			Clicked(this);
		}
		setFocus(false);
	}
	return true;
}

void Button::render() {
	Widget::renderBackground();
	if (TextWidget::hasText()) {
		TextWidget::renderText();
	}
	Widget::renderForeground();
}

// =====================================================
//  class CheckBox
// =====================================================

CheckBox::CheckBox(Container* parent)
		: Button(parent), m_checked(false)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::CHECK_BOX);
	setSize(getPrefSize());
}

CheckBox::CheckBox(Container* parent, Vec2i pos, Vec2i size)
		: Button(parent, pos, size, false), m_checked(false)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::CHECK_BOX);
}

void CheckBox::setSize(const Vec2i &sz) {
	// bypass Button::setSize()
	Widget::setSize(sz);
	//int y = int((sz.y - getTextFont()->getMetrics()->getHeight()) / 2);
	//setTextPos(Vec2i(40, y), 0);
	//setTextPos(Vec2i(40, y), 1);
}

Vec2i CheckBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	Vec2i res = txtDim + xtra + Vec2i(txtDim.y + 2, 0);
	return res;
}

Vec2i CheckBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	dim.x += 40;
	if (dim.y < 32) {
		dim.y = 32;
	}
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	return dim + xtra;
}

bool CheckBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		setFocus(true);
		return true;
	}
	return false;
}

bool CheckBox::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (isFocused() && isHovered()) {
			m_checked = !m_checked;
			Clicked(this);
		}
		setFocus(false);
		return true;
	}
	return false;
}

// =====================================================
//  class TextBox
// =====================================================

TextBox::TextBox(Container* parent)
		: Widget(parent)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, changed(false) {
	setWidgetStyle(WidgetType::TEXT_BOX);
}

TextBox::TextBox(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, changed(false) {
	setWidgetStyle(WidgetType::TEXT_BOX);
}

void TextBox::gainFocus() {
	if (isEnabled()) {
		setFocus(true);
		getRootWindow()->aquireKeyboardFocus(this);
	}
}

bool TextBox::mouseDown(MouseButton btn, Vec2i pos) {
	gainFocus();
	return true;
}

bool TextBox::mouseUp(MouseButton btn, Vec2i pos) {
	return true;
}

bool TextBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::BACK_SPACE: {
			const string &txt = getText();
			if (!txt.empty()) {
				setText(txt.substr(0, txt.size() - 1));
				TextChanged(this);
			}
			return true;
		}
		case KeyCode::RETURN:
			getRootWindow()->releaseKeyboardFocus(this);
			InputEntered(this);
			return true;
		
		// need to let ESC 'go through' (return false, so the owning dialog gets it)
		// maybe should be a flag? (for non-dialog parents)
		case KeyCode::ESCAPE:
			//getRootWindow()->releaseKeyboardFocus(this);
			//return true;
			/*
		case KeyCode::DELETE_:
		case KeyCode::ARROW_LEFT:
		case KeyCode::ARROW_RIGHT:
		case KeyCode::HOME:
		case KeyCode::END:
		case KeyCode::TAB:
			cout << "KeyDown: [" << KeyCodeNames[code] << "]\n";
			return true;
			*/
		default:
			break;
	}
	return false;
}

bool TextBox::keyUp(Key key) {
	return false;
}

bool TextBox::keyPress(char c) {
	if (c >= 32 && c <= 126) { // 'space' -> 'tilde' [printable ascii char]
		if (!m_inputMask.empty()) {
			if (m_inputMask.find_first_of(c) == string::npos) {
				return true; //  consume dis-allowed char
			}
		}
		string s(getText());
		setText(s + c);
		TextChanged(this);
		return true; //  consume key press
	}
	return false; // let other stuff through
}

void TextBox::lostKeyboardFocus() {
	setFocus(false);
}

void TextBox::render() {
	Widget::renderBackground();
	TextWidget::renderText();
	Widget::renderForeground();
}

Vec2i TextBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	return Vec2i(200, txtDim.y) + m_borderStyle.getBorderDims();
}

Vec2i TextBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	return dim + m_borderStyle.getBorderDims();
}

// =====================================================
// class CellStrip
// =====================================================

CellStrip::CellStrip(WidgetWindow *window, Orientation ortn, Origin orgn, int cells)
		: Container(window)
		, m_orientation(ortn)
		, m_origin(orgn)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		//m_cells.push_back(new WidgetCell(this));
		m_cells2.push_back(CellInfo());
	}
}

CellStrip::CellStrip(Container *parent, Orientation ortn)
		: Container(parent)
		, m_orientation(ortn)
		, m_origin(Origin::CENTRE)
		, m_dirty(false) {
}

CellStrip::CellStrip(Container *parent, Orientation ortn, int cells)
		: Container(parent)
		, m_orientation(ortn)
		, m_origin(Origin::CENTRE)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		//m_cells.push_back(new WidgetCell(this));
		m_cells2.push_back(CellInfo());
	}
}

CellStrip::CellStrip(Container *parent, Orientation ortn, Origin orgn, int cells)
		: Container(parent)
		, m_orientation(ortn)
		, m_origin(orgn)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		//m_cells.push_back(new WidgetCell(this));
		m_cells2.push_back(CellInfo());
	}
}

CellStrip::CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ortn) 
		: Container(parent, pos, size)
		, m_orientation(ortn)
		, m_origin(Origin::CENTRE)
		, m_dirty(false) {
}

CellStrip::CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ortn, Origin orgn, int cells) 
		: Container(parent, pos, size)
		, m_orientation(ortn)
		, m_origin(orgn)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		//m_cells.push_back(new WidgetCell(this));
		m_cells2.push_back(CellInfo());
	}
}

//void CellStrip::setCustumCell(int ndx, WidgetCell *cell) {
//	ASSERT_RANGE(ndx, m_cells.size());
//	WidgetCell *old = m_cells[ndx];
//	m_cells[ndx] = cell;
//	m_children[ndx] = cell;
//	delete old;
//}

void CellStrip::addChild(Widget *child) {
	Container::addChild(child);
}

//void CellStrip::clearCells() {
//	foreach (vector<WidgetCell*>, it, m_cells) {
//		(*it)->clear();
//	}
//}
//
//void CellStrip::deleteCells() {
//	foreach (vector<WidgetCell*>, it, m_cells) {
//		delete *it;
//	}
//	m_cells.clear();
//}

void CellStrip::addCells(int n) {
	for (int i=0; i < n; ++i) {
		m_cells2.push_back(CellInfo());
	}
}

void CellStrip::setPos(const Vec2i &pos) {
	Container::setPos(pos);
	setDirty();
}

void CellStrip::setSize(const Vec2i &sz) {
	Container::setSize(sz);
	//layoutCells();
	setDirty();
}

void CellStrip::render(bool clip) {
	assert(glIsEnabled(GL_BLEND));
	if (m_dirty) {
		WIDGET_LOG( descShort() << " : CellStrip::render() : dirty, laying out cells." );
		layoutCells();
	}
	Widget::render();

	if (clip) { // clip children
		Vec2i pos = getScreenPos();
		pos.x += getBorderLeft();
		pos.y += getBorderTop();
		Vec2i size = getSize() - m_borderStyle.getBorderDims();
		m_rootWindow->pushClipRect(pos, size);
	}
	if (m_orientation == Orientation::VERTICAL) {
		foreach (WidgetList, it, m_children) {
			Widget* widget = *it;
			if (widget->isVisible() && widget->getPos().y < getSize().h
			&& widget->getPos().y + widget->getSize().h >= 0 ) {
				widget->render();
			}
		}
	} else { // Orientation::HORIZONTAL
		assert(m_orientation == Orientation::HORIZONTAL);
		foreach (WidgetList, it, m_children) {
			Widget* widget = *it;
			if (widget->isVisible() && widget->getPos().x < getSize().w
			&& widget->getPos().x + widget->getSize().w >= 0 ) {
				widget->render();
			}
		}
	}
	if (clip) {
		m_rootWindow->popClipRect();
	}
}

void CellStrip::render() {
	render(true);
}

typedef vector<SizeHint>    HintList;
typedef pair<int, int>      CellDim;
typedef vector<CellDim>     CellDimList;

/** splits up an amount of space (single dimension) according to a set of hints
  * @param hints the list of hints, one for each cell
  * @param space the amount of space to split up
  * @param out_res result vector, each entry is an offset and size pair
  * @return the amount of space that is "left over" (not allocated to a cell)
  */
int calculateCellDims(HintList &hints, const int space, CellDimList &out_res) {
	RUNTIME_CHECK_MSG(space > 0, "calculateCellDims(): called with no space.");
	RUNTIME_CHECK_MSG(out_res.empty(), "calculateCellDims(): output vector not empty!");
	if (hints.empty()) {
		return 0; // done ;)
	}
	const int count = hints.size();

	// Pass 1
	// count number of percentage hints and number of them that are 'default', and determine 
	// space for percentage hinted cells (ie, subtract space taken by absolute hints)
	int numPcnt = 0;
	int numDefPcnt = 0;
	int pcntTally = 0;
	int pcntSpace = space;
	foreach_const (HintList, it, hints) {
		if (it->isPercentage()) {
			if (it->getPercentage() >= 0) {
				pcntTally += it->getPercentage();
			} else {
				++numDefPcnt;
			}
			++numPcnt;
		} else {
			pcntSpace -= it->getAbsolute();
		}
	}

	float percent = pcntSpace / 100.f;  // pixels per percent
	int defPcnt;
	if (100 - pcntTally > 0 && numDefPcnt) { // default percentage hints get this much...
		defPcnt = (100 - pcntTally) / numDefPcnt;
	} else {
		defPcnt = 0;
	}

	// Pass 2
	// convert percentages and write cell sizes to output vector
	int offset = 0, size;
	foreach_const (HintList, it, hints) {
		if (it->isPercentage()) {
			if (it->getPercentage() >= 0) {
				size = int(it->getPercentage() * percent);
			} else {
				size = int(defPcnt * percent);
			}
		} else {
			size = it->getAbsolute();
		}
		out_res.push_back(std::make_pair(offset, size));
		offset += size;
	}
	return space - offset;
}

void CellStrip::layoutCells() {
	WIDGET_LOG( descLong() << " : CellStrip::layoutCells()" );
	m_dirty = false;
	// collect hints
	HintList     hintList;
	foreach (CellInfos, it, m_cells2) {
		hintList.push_back(it->m_hint);
	}
	// determine space available
	int space;
	if (m_orientation == Orientation::VERTICAL) {
		space = getHeight() - getBordersVert();
	} else if (m_orientation == Orientation::HORIZONTAL) {
		space = getWidth() - getBordersHoriz();
	} else {
		throw runtime_error("WidgetStrip has invalid direction.");
	}
	if (space < 1) {
		return;
	}

	// split space according to hints
	CellDimList  resultList;
	int offset;
	int spare = calculateCellDims(hintList, space, resultList);
	if (m_origin == Origin::FROM_TOP || m_origin == Origin::FROM_LEFT) {
		offset = 0;
	} else if (m_origin == Origin::CENTRE) {
		offset = spare / 2;
	} else {
		offset = space - spare;
	}

	// determine cell width and x-pos OR height and y-pos
	int ppos, psize;
	if (m_orientation == Orientation::VERTICAL) {
		ppos = getBorderLeft();
		psize = getWidth() - getBordersHoriz();
	} else {
		ppos = getBorderTop();
		psize = getHeight() - getBordersVert();
	}
	// combine results and set cell pos and size
	Vec2i pos, size;
	for (int i=0; i < m_cells2.size(); ++i) {
		if (m_orientation == Orientation::VERTICAL) {
			pos = Vec2i(ppos, getBorderTop() + offset + resultList[i].first);
			size = Vec2i(psize, resultList[i].second);
		} else {
			pos = Vec2i(getBorderLeft() + offset + resultList[i].first, ppos);
			size = Vec2i(resultList[i].second, psize);
		}
		WIDGET_LOG( descShort() << " : CellStrip::layoutCells(): setting cell " << i 
			<< " pos: " << pos << ", size = " << size );

		m_cells2[i].m_pos = pos;
		m_cells2[i].m_size = size;
	}
	
	foreach (WidgetList, it, m_children) {
		(*it)->anchor();
	}
}

// =====================================================
//  class Panel
// =====================================================
//
//Panel::Panel(Container* parent)
//		: Container(parent)
//		, autoLayout(true)
//		, layoutOrigin(Origin::CENTRE) {
//	setPaddingParams(10, 5);
//	m_borderStyle.setNone();
//}
//
//Panel::Panel(Container* parent, Vec2i pos, Vec2i sz)
//		: Container(parent, pos, sz)
//		, autoLayout(true)
//		, layoutOrigin(Origin::CENTRE) {
//	setPaddingParams(10, 5);
//	m_borderStyle.setNone();
//}
//
//Panel::Panel(WidgetWindow* window)
//		: Container(window) {
//}
//
//void Panel::setPaddingParams(int panelPad, int widgetPad) {
//	//setPadding(panelPad);
//	widgetPadding = widgetPad;
//}
//
//void Panel::setLayoutParams(bool autoLayout, Orientation dir, Origin origin) {
//	assert(
//		origin == Origin::CENTRE
//		|| ((origin == Origin::FROM_BOTTOM || origin == Origin::FROM_TOP)
//			&& dir == Orientation::VERTICAL)
//		|| ((origin == Origin::FROM_LEFT || origin == Origin::FROM_RIGHT)
//			&& dir == Orientation::HORIZONTAL)
//	);
//	this->layoutDirection = dir;
//	this->layoutOrigin = origin;		
//	this->autoLayout = autoLayout;
//}
//
//void Panel::layoutChildren() {
//	if (!autoLayout || m_children.empty()) {
//		return;
//	}
//	if (layoutDirection == Orientation::VERTICAL) {
//		layoutVertical();
//	} else if (layoutDirection == Orientation::HORIZONTAL) {
//		layoutHorizontal();
//	}
//}
//
//void Panel::layoutVertical() {
//	vector<int> widgetYPos;
//	int wh = 0;
//	Vec2i size = getSize();
//	Vec2i room = size - m_borderStyle.getBorderDims();
//	foreach (WidgetList, it, m_children) {
//		wh +=  + widgetPadding;
//		widgetYPos.push_back(wh);
//		wh += (*it)->getHeight();
//	}
//	wh -= widgetPadding;
//	
//	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT], 
//		getPos().y + m_borderStyle.m_sizes[Border::TOP]);
//	
//	int offset;
//	switch (layoutOrigin) {
//		case Origin::FROM_TOP: offset = m_borderStyle.m_sizes[Border::TOP]; break;
//		case Origin::CENTRE: offset = (size.y - wh) / 2; break;
//		case Origin::FROM_BOTTOM: offset = size.y - wh - m_borderStyle.m_sizes[Border::TOP]; break;
//	}
//	int ndx = 0;
//	foreach (WidgetList, it, m_children) {
//		int ww = (*it)->getWidth();
//		int x = topLeft.x + (room.x - ww) / 2;
//		int y = offset + widgetYPos[ndx++];
//		(*it)->setPos(x, y);
//	}
//}
//
//void Panel::layoutHorizontal() {
//	vector<int> widgetXPos;
//	int ww = 0;
//	Vec2i size = getSize();
//	Vec2i room = size - m_borderStyle.getBorderDims();
//	foreach (WidgetList, it, m_children) {
//		widgetXPos.push_back(ww);
//		ww += (*it)->getWidth();
//		ww += widgetPadding;
//	}
//	ww -= widgetPadding;
//	
//	Vec2i topLeft(getBorderLeft(), size.y - getBorderTop());
//	
//	int offset;
//	switch (layoutOrigin) {
//		case Origin::FROM_LEFT: offset = getBorderLeft(); break;
//		case Origin::CENTRE: offset = (size.x - ww) / 2; break;
//		case Origin::FROM_RIGHT: offset = size.x - ww - getBorderRight(); break;
//	}
//	int ndx = 0;
//	foreach (WidgetList, it, m_children) {
//		int wh = (*it)->getHeight();
//		int x = offset + widgetXPos[ndx++];
//		int y = (room.y - wh) / 2;
//		(*it)->setPos(x, y);
//	}
//}
//
//Vec2i Panel::getMinSize() const {
//	return Vec2i(-1);
//}
//
//Vec2i Panel::getPrefSize() const {
//	return Vec2i(-1);
//}
//
//void Panel::addChild(Widget* child) {
//	Container::addChild(child);
//	if (!autoLayout) {
//		return;
//	}
//	Vec2i sz = child->getSize();
//	int space_x = getWidth() - m_borderStyle.getHorizBorderDim() * 2;
//	if (sz.x > space_x) {
//		child->setSize(space_x, sz.y);
//	}
//	layoutChildren();
//}
//
//void Panel::render() {
//	assertGl();
//	Vec2i pos = getScreenPos();
//	pos.x += getBorderLeft();
//	pos.y += getBorderTop();
//	Vec2i size = getSize() - m_borderStyle.getBorderDims();
//	m_rootWindow->pushClipRect(pos, size);
//	Container::render();
//	m_rootWindow->popClipRect();
//}

// =====================================================
//  class PicturePanel
// =====================================================

Vec2i PicturePanel::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = m_borderStyle.getBorderDims();
	return imgDim + xtra;
}

Vec2i PicturePanel::getPrefSize() const {
	return Vec2i(-1);
}

// =====================================================
//  class OptionWidget
// =====================================================

OptionWidget::OptionWidget(Container *parent, const string &text)
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
	label->setAlignment(Alignment::FLUSH_RIGHT);
	label->borderStyle().setSizes(0, 0, 10, 0);
}

void OptionWidget::setOptionWidget(Widget *widget) {
	widget->setCell(1);
	widget->setAnchors(Anchors::getFillAnchors());
}

void OptionWidget::setPercentSplit(int lblPercent) {
	setSizeHint(0, SizeHint(lblPercent));
	setSizeHint(1, SizeHint(100 - lblPercent));
}

void OptionWidget::setAbsoluteSplit(int val, bool label) {
	if (label) {
		setSizeHint(0, SizeHint(0, val));
		setSizeHint(1, SizeHint(-1));
	} else {
		setSizeHint(0, SizeHint(-1));
		setSizeHint(1, SizeHint(0, val));
	}
}

// =====================================================
//  class DoubleOption
// =====================================================

DoubleOption::DoubleOption(Container *parent, const string &txt1, const string &txt2)
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
	label->setAlignment(Alignment::FLUSH_RIGHT);
	label->borderStyle().setSizes(0, 0, 10, 0);

	label = new StaticText(this);
	label->setText(txt2);
	label->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	label->setCell(2);
	label->setAnchors(Anchor(AnchorType::RIGID, 0));
	label->setAlignment(Alignment::FLUSH_RIGHT);
	label->borderStyle().setSizes(0, 0, 10, 0);
}

void DoubleOption::setOptionWidget(bool first, Widget *widget) {
	if (first) {
		widget->setCell(1);
	} else {
		widget->setCell(3);
	}
	widget->setAnchors(Anchors::getFillAnchors());
}

void DoubleOption::setCustomSplit(bool first, int label) {
	assert(label > 0 && label < 50);
	int content = 50 - label;
	int i = first ? 0 : 2;
	setSizeHint(i++, SizeHint(label));
	setSizeHint(i, SizeHint(content));
}

// =====================================================
//  class TabWidget
// =====================================================

TabWidget::TabWidget(Container *parent)
		: CellStrip(parent, Orientation::VERTICAL, 2)
		, m_active(0)
		, m_anchors(Anchor(AnchorType::RIGID, 0)) {
	setAnchors(m_anchors);
	m_btnPnl = new CellStrip(this, Orientation::HORIZONTAL, Origin::FROM_LEFT);
	m_btnPnl->setCell(0);
	m_btnPnl->setAnchors(m_anchors);
	m_btnPnl->setSize(Vec2i(g_config.getDisplayWidth(), g_widgetConfig.getDefaultItemHeight()));
	//m_btnPnl->borderStyle().setSolid(g_widgetConfig.getColourIndex(Vec3f(1.f, 0.f, 1.f)));
	//m_btnPnl->borderStyle().setSizes(2);
}

int TabWidget::getButtonPos(Button *button) {
	for (int i = 0; i < m_pages.size(); ++i) {
		if (m_buttons[i] == button) {
			return i; // button found
		}
	}
	return -1; // button not found
}

void TabWidget::onButtonClicked(Widget* widget) {
	// find the index of the button to get the associated page
	int index = getButtonPos(static_cast<Button*>(widget));

	// otherwise callback was connected to wrong widget
	assert(index >= 0);

	if (m_active != index) {
		// hide the previous page and show the current one
		m_pages[m_active]->setVisible(false);
		m_pages[index]->setVisible(true);

		// deselect the previous tab and select the current one
		m_buttons[m_active]->setSelected(false);
		m_buttons[index]->setSelected(true);

		m_active = index;
	}
}

void TabWidget::setActivePage(int index) {
	if (m_active != index) {
		// hide the previous page and show the current one
		m_pages[m_active]->setVisible(false);
		m_pages[index]->setVisible(true);

		// deselect the previous button and select the current one
		m_buttons[m_active]->setSelected(false);
		m_buttons[index]->setSelected(true);

		m_active = index;
	}
}

Button* TabWidget::createButton(const string &text) {
	int s = g_widgetConfig.getDefaultItemHeight();
	m_btnPnl->addCells(1);
	m_btnPnl->setSizeHint(m_buttons.size(), SizeHint(-1, g_widgetConfig.getDefaultItemHeight() * 4));
	Vec2i pos(0,0);
	Vec2i size(g_widgetConfig.getDefaultItemHeight() * 4, g_widgetConfig.getDefaultItemHeight());
	Button *btn = new Button(m_btnPnl, pos, size);
	btn->setText(text);
	btn->setCell(m_buttons.size());
	btn->setAnchors(Anchors::getFillAnchors());
	btn->Clicked.connect(this, &TabWidget::onButtonClicked);
	return btn;
}

void TabWidget::add(const string &text, CellStrip *cellStrip) {
	assert(cellStrip);
	int index = m_buttons.size();

	// only add unique buttons since only the first one would
	// be used.
	//if (index == 0 || getButtonPos(button) == -1) {
		cellStrip->setCell(1);
		cellStrip->setAnchors(Anchors::getFillAnchors());
	//cellStrip->layoutCells();
		m_buttons.push_back(createButton(text));
		m_pages.push_back(cellStrip);
		
		// show the first page
		if (index == 0) {
			cellStrip->setVisible(true);
			m_buttons[index]->setSelected(true);
		} else {
			cellStrip->setVisible(false);
		}

		assert(m_buttons.size() == index + 1);
		assert(m_pages.size() == index + 1);
	//}

	layoutCells();
}

}}

