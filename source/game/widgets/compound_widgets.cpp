// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010-2011 James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "compound_widgets.h"
#include "widget_window.h"
#include "core_data.h"
#include "lang.h"
#include "leak_dumper.h"
#include "config.h"
#include "renderer.h"

#include <list>

namespace Glest { namespace Widgets {
using Shared::Util::intToStr;
using namespace Global;
using namespace Shared::Graphics::Gl;


// =====================================================
//  class OptionContainer
// =====================================================

OptionContainer::OptionContainer(Container* parent, Vec2i pos, Vec2i size, const string &text)
		: Container(parent, pos, size) {
	setWidgetStyle(WidgetType::TEXT_BOX);
	m_abosulteLabelSize = false;
	m_labelSize = 30;
	int w = int(size.x * float(30) / 100.f);
	m_label = new StaticText(this, Vec2i(0), Vec2i(w, size.y));
	m_label->setText(text);
	m_widget = 0;
}

void OptionContainer::setWidget(Widget* widget) {
	m_widget = widget;
	Vec2i size = getSize();
	int w;
	if (m_abosulteLabelSize) {
		w = m_labelSize;
	} else {
		w = int(size.x * float(m_labelSize) / 100.f);
	}
	m_label->setSize(Vec2i(w, size.y));
	if (m_widget) {
		m_widget->setPos(Vec2i(w,0));
		m_widget->setSize(Vec2i(size.x - w, size.y));
	}
}

void OptionContainer::setLabelWidth(int value, bool absolute) {
	m_abosulteLabelSize = absolute;
	m_labelSize = value;
	Vec2i size = getSize();
	int w;
	if (m_abosulteLabelSize) {
		w = m_labelSize;
	} else {
		w = int(size.x * float(m_labelSize) / 100.f);
	}
	m_label->setSize(Vec2i(w, size.y));
	if (m_widget) {
		m_widget->setPos(Vec2i(w,0));
		m_widget->setSize(Vec2i(size.x - w, size.y));
	}
}

Vec2i OptionContainer::getPrefSize() const {
	return Vec2i(700, 40);
}

Vec2i OptionContainer::getMinSize() const {
	return Vec2i(400, 30);
}

// =====================================================
//  class ScrollText
// =====================================================

ScrollText::ScrollText(Container* parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 1)
		, TextWidget(this) {
	init();
}

ScrollText::ScrollText(Container* parent, Vec2i pos, Vec2i size)
		: CellStrip(parent, pos, size, Orientation::HORIZONTAL, Origin::CENTRE, 1)
		, TextWidget(this) {
	init();
	recalc();
}

void ScrollText::init() {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::TEXT_BOX));

	int itemSize = m_rootWindow->getConfig()->getDefaultItemHeight();

	// Anchors for scroll-bar, stick to the top, right & bottom sides.
	// Not anchored to left border, so we must set size (width will be respected, height will not)
	Anchors anchors(Anchor(AnchorType::NONE, 0), Anchor(AnchorType::RIGID, 0), // left, top
		Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0));           // right, bottom
	
	m_scrollBar = new ScrollBar(this, true, 10);
	m_scrollBar->setCell(0);
	m_scrollBar->setAnchors(anchors);
	m_scrollBar->setSize(Vec2i(itemSize));
	m_scrollBar->ThumbMoved.connect(this, &ScrollText::onScroll);

	// Anchors for text widget, stick to left, top & bottom, and 'itemSize' in from right
	anchors = Anchors(Anchor(AnchorType::RIGID, 0), Anchor(AnchorType::RIGID, 0),
		Anchor(AnchorType::RIGID, itemSize), Anchor(AnchorType::RIGID, 0));

	m_staticText = new StaticText(this);
	m_staticText->setCell(0);
	m_staticText->setAnchors(anchors);
	m_staticText->setAlignment(Alignment::NONE);
	m_staticText->setText("");
}

void ScrollText::recalc() {
	int th = m_staticText->getTextDimensions().h;
	int ch = m_staticText->getHeight() - m_staticText->getBordersVert();
	m_scrollBar->setRanges(th, ch);
}

void ScrollText::onScroll(ScrollBar*) {
	int ox = m_staticText->getPos().x;
	m_staticText->setPos(Vec2i(ox, -round(m_scrollBar->getThumbPos())));
}

void ScrollText::setSize(const Vec2i &sz) {
	const FontMetrics *fm = g_widgetConfig.getFont(m_staticText->textStyle().m_fontIndex)->getMetrics();
	CellStrip::setSize(sz);
	layoutCells(); // force
	if (!m_origString.empty()) {
		string text = m_origString;
		int width = m_staticText->getSize().w - m_staticText->getBordersHoriz();
		fm->wrapText(text, width);	
		m_staticText->setText(text);
	}
	recalc();
}

void ScrollText::setText(const string &txt, ScrollAction scroll) {
	float oldOffset = m_scrollBar->getThumbPos();
	const FontMetrics *fm = m_staticText->getFont()->getMetrics();
	int width = getSize().w - m_scrollBar->getSize().w - getBordersHoriz() - m_staticText->getBordersHoriz();
	m_origString = txt;
	string text = txt;
	fm->wrapText(text, width);
	
	m_staticText->setText(text);
	recalc();
	
	if (scroll == ScrollAction::BOTTOM) {
		m_scrollBar->setThumbPosPercent(100);
	} else if (scroll == ScrollAction::MAINTAIN) {
		//WIDGET_LOG( "ScrollText::setText() : Restoring offset: " << oldOffset );
		m_scrollBar->setThumbPos(oldOffset);
	} else {
		m_scrollBar->setThumbPos(0);
	}
}

void ScrollText::render() {
	CellStrip::render();
	assertGl();
}

// =====================================================
//  class TitleBar
// =====================================================

TitleBar::TitleBar(Container* parent, ButtonFlags flags)
		: CellStrip(parent, Orientation::HORIZONTAL)
		, m_titleText(0)
		, m_closeButton(0)
		, m_rollUpButton(0)
		, m_rollDownButton(0)
		, m_expandButton(0)
		, m_shrinkButton(0) {
	setWidgetStyle(WidgetType::TITLE_BAR);
	init(flags);
}

//TitleBar::TitleBar(Container* parent, ButtonFlags flags, Vec2i pos, Vec2i size, string title)
//		: CellStrip(parent, pos, size, Orientation::HORIZONTAL)
//		, m_titleText(0)
//		, m_closeButton(0)
//		, m_rollUpButton(0)
//		, m_rollDownButton(0)
//		, m_expandButton(0)
//		, m_shrinkButton(0) {
//	setWidgetStyle(WidgetType::TITLE_BAR);
//	init(flags);
//	m_titleText->setText(title);
//}

void TitleBar::init(ButtonFlags flags) {
	m_flags = flags;
	int n = flags.getCount() + 1;
	addCells(n);
	if (getHeight()) {
		setSizeHints();
	}
	Anchors anchors(Anchor(AnchorType::RIGID, 2));
	m_titleText = new StaticText(this);
	m_titleText->setCell(0);
	m_titleText->setAnchors(anchors);

	int cell = 1;
	if (flags.isSet(ButtonFlags::ROLL_UPDOWN)) {
		m_rollDownButton = new RollDownButton(this);
		m_rollDownButton->setCell(cell);
		m_rollDownButton->setAnchors(anchors);
		m_rollDownButton->Clicked.connect(this, &TitleBar::onButtonClicked);
		m_rollDownButton->setVisible(false);

		m_rollUpButton = new RollUpButton(this);
		m_rollUpButton->setCell(cell++);
		m_rollUpButton->setAnchors(anchors);
		m_rollUpButton->Clicked.connect(this, &TitleBar::onButtonClicked);
		m_rollUpButton->setVisible(true);
	}
	if (flags.isSet(ButtonFlags::SHRINK)) {
		m_shrinkButton = new ShrinkButton(this);
		m_shrinkButton->setCell(cell++);
		m_shrinkButton->setAnchors(anchors);
		m_shrinkButton->Clicked.connect(this, &TitleBar::onButtonClicked);
	}
	if (flags.isSet(ButtonFlags::EXPAND)) {
		m_expandButton = new ExpandButton(this);
		m_expandButton->setCell(cell++);
		m_expandButton->setAnchors(anchors);
		m_expandButton->Clicked.connect(this, &TitleBar::onButtonClicked);
	}
	if (flags.isSet(ButtonFlags::CLOSE)) {
		m_closeButton = new CloseButton(this);
		m_closeButton->setCell(cell++);
		m_closeButton->setAnchors(anchors);
		m_closeButton->Clicked.connect(this, &TitleBar::onButtonClicked);
	}
}

void TitleBar::onButtonClicked(Widget *source) {
	Button *btn = static_cast<Button*>(source);
	if (btn == m_rollDownButton) {
		RollDown(this);
	} else if (btn == m_rollUpButton) {
		RollUp(this);
	} else if (btn == m_shrinkButton) {
		Shrink(this);
	} else if (btn == m_expandButton) {
		Expand(this);
	} else if (btn == m_closeButton) {
		Close(this);
	}
}

void TitleBar::setSizeHints() {
	SizeHint hint(-1, getHeight());
	const int n = m_flags.getCount() + 1;
	for (int i=1; i < n; ++i) {
		setSizeHint(i, hint);
	}
}

// =====================================================
//  class Frame
// =====================================================

Frame::Frame(WidgetWindow *ww, ButtonFlags flags)
		: CellStrip(ww, Orientation::VERTICAL, Origin::FROM_TOP, 2)
		, MouseWidget(this) {
	init(flags);
}

Frame::Frame(Container *parent, ButtonFlags flags)
		: CellStrip(parent, Orientation::VERTICAL, Origin::FROM_TOP, 2)
		, MouseWidget(this) {
	init(flags);
}

Frame::Frame(Container *parent, ButtonFlags flags, Vec2i pos, Vec2i sz)
		: CellStrip(parent, pos, sz, Orientation::VERTICAL, Origin::FROM_TOP, 2)
		, MouseWidget(this) {
	init(flags);
}

void Frame::init(ButtonFlags flags) {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));

	m_pressed = m_rollingUp = m_rollingDown = m_rolledUp = false;
	m_resizerAction = 0;

	Anchors anchors(Anchor(AnchorType::RIGID, 0));
	m_titleBar = new TitleBar(this, flags);
	m_titleBar->setCell(0);
	m_titleBar->setAnchors(anchors);
	setSizeHint(0, SizeHint(-1, g_widgetConfig.getDefaultItemHeight() + 4));

	m_titleBar->Close.connect(this, &Frame::onClose);
	m_titleBar->RollUp.connect(this, &Frame::onRollUp);
	m_titleBar->RollDown.connect(this, &Frame::onRollDown);
	m_titleBar->Shrink.connect(this, &Frame::onShrink);
	m_titleBar->Expand.connect(this, &Frame::onExpand);
}

void Frame::onRollUp(Widget*) {
	assert(!m_rollingUp && !m_rollingDown && !m_rolledUp && !m_resizerAction);
	m_origSize = getSize();
	Vec2i newSize = getBordersAll() + m_titleBar->getSize();
	m_titleBar->disableRollUpDown();
	m_resizerAction = new ResizeWidgetAction(120, this);
	m_resizerAction->setPosTransition(Vec2f(m_origSize), Vec2f(newSize), TransitionFunc::LOGARITHMIC);
	m_rootWindow->registerUpdate(this);
	m_rollingUp = true;
}

void Frame::onRollDown(Widget*) {
	assert(!m_rollingUp && !m_rollingDown && m_rolledUp && !m_resizerAction);
	Vec2i startSize = getSize();
	m_titleBar->disableRollUpDown();
	m_resizerAction = new ResizeWidgetAction(120, this);
	m_resizerAction->setPosTransition(Vec2f(startSize), Vec2f(m_origSize), TransitionFunc::LOGARITHMIC);
	m_rootWindow->registerUpdate(this);
	m_rollingDown = true;
}

void Frame::update() {
	assert(m_resizerAction && (m_rollingUp || m_rollingDown));
	if (m_resizerAction->update()) {
		if (m_rollingUp) {
			m_rolledUp = true;
			m_rollingUp = false;
		} else {
			m_rolledUp = false;
			m_rollingDown = false;
		}
		m_titleBar->swapRollUpDown();
		delete m_resizerAction;
		m_resizerAction = 0;
		m_rootWindow->unregisterUpdate(this);
	}
}

void Frame::setTitleText(const string &text) {
	m_titleBar->setText(text);
}

bool Frame::mouseDown(MouseButton btn, Vec2i pos) {
	if (m_titleBar->isInside(pos) && btn == MouseButton::LEFT) {
		m_pressed = true;
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool Frame::mouseMove(Vec2i pos) {
	if (m_pressed) {
		Vec2i offset = pos - m_lastPos;
		Vec2i newPos = getPos() + offset;
		setPos(newPos);
		m_lastPos = pos;
		return true;
	}
	return false;
}

bool Frame::mouseUp(MouseButton btn, Vec2i pos) {
	if (m_pressed && btn == MouseButton::LEFT) {
		m_pressed = false;
		return true;
	}
	return false;
}

// =====================================================
//  class BasicDialog
// =====================================================

BasicDialog::BasicDialog(WidgetWindow* window)
		: Frame(window, ButtonFlags::CLOSE), m_content(0)
		, m_button1(0), m_button2(0), m_buttonCount(0), m_btnPnl(0) {
	init();
}

BasicDialog::BasicDialog(WidgetWindow* window, ButtonFlags flags)
		: Frame(window, flags), m_content(0)
		, m_button1(0) , m_button2(0), m_buttonCount(0), m_btnPnl(0) {
	init();
}

BasicDialog::BasicDialog(Container* parent)
		: Frame(parent, ButtonFlags::CLOSE), m_content(0)
		, m_button1(0), m_button2(0), m_buttonCount(0), m_btnPnl(0) {
	init();
}

BasicDialog::BasicDialog(Container* parent, ButtonFlags flags)
		: Frame(parent, flags), m_content(0)
		, m_button1(0) , m_button2(0), m_buttonCount(0), m_btnPnl(0) {
	init();
}

void BasicDialog::init() {
	setStyle(g_widgetConfig.getWidgetStyle(WidgetType::MESSAGE_BOX));
	addCells(1);
}

void BasicDialog::init(Vec2i pos, Vec2i size, const string &title, 
					   const string &btn1Text, const string &btn2Text) {
	setPos(pos);
	setSize(size);
	setTitleText(title);
	setButtonText(btn1Text, btn2Text);
	layoutCells();
}

void BasicDialog::setContent(Widget* content) {
	assert(content->getParent() == this);
	m_content = content;
	m_content->setCell(1);
	m_content->setAnchors(Anchors(Anchor(AnchorType::RIGID, 0)));
	setDirty();
}

void BasicDialog::setButtonText(const string &btn1Text, const string &btn2Text) {
	delete m_btnPnl;  m_btnPnl  = 0;
	delete m_button1; m_button1 = 0;
	delete m_button2; m_button2 = 0;

	m_buttonCount = btn2Text.empty() ? (btn1Text.empty() ? 0 : 1) : 2;
	if (m_buttonCount) {
		setSizeHint(2, SizeHint(-1, g_widgetConfig.getDefaultItemHeight() * 3 / 2));
		m_btnPnl = new CellStrip(this, Orientation::HORIZONTAL, m_buttonCount);
		m_btnPnl->setCell(2);
		Anchors anchors(Anchor(AnchorType::RIGID, 0));
		m_btnPnl->setAnchors(anchors);

		Vec2i pos(0,0);
		Vec2i size(g_widgetConfig.getDefaultItemHeight() * 5, g_widgetConfig.getDefaultItemHeight());
		anchors.setCentre(true);
		m_button1 = new Button(m_btnPnl, pos, size);
		m_button1->setCell(0);
		m_button1->setAnchors(anchors);
		m_button1->setText(btn1Text);
		m_button1->Clicked.connect(this, &BasicDialog::onButtonClicked);

		if (m_buttonCount == 2) {
			m_button2 = new Button(m_btnPnl, pos, size);
			m_button2->setCell(1);
			m_button2->setAnchors(anchors);
			m_button2->setText(btn2Text);
			m_button2->Clicked.connect(this, &BasicDialog::onButtonClicked);
		}
	} else {
		setSizeHint(2, SizeHint(-1, 0));
	}
	setDirty();
}

void BasicDialog::onButtonClicked(Widget *source) {
	Button *btn = static_cast<Button*>(source);
	if (btn == m_button1) {
		Button1Clicked(this);
	} else {
		Button2Clicked(this);
	}
}

// =====================================================
//  class MessageDialog
// =====================================================

MessageDialog::MessageDialog(WidgetWindow* window)
		: BasicDialog(window) {
	m_scrollText = new ScrollText(this);
	setContent(m_scrollText);
}

MessageDialog::MessageDialog(Container* parent)
		: BasicDialog(parent, ButtonFlags::CLOSE | ButtonFlags::ROLL_UPDOWN) {
	m_scrollText = new ScrollText(this);
	setContent(m_scrollText);
}

MessageDialog* MessageDialog::showDialog(Vec2i pos, Vec2i size, const string &title, 
								const string &msg,  const string &btn1Text, const string &btn2Text) {
	MessageDialog* msgBox = new MessageDialog(&g_widgetWindow);
	g_widgetWindow.setFloatingWidget(msgBox, true);
	msgBox->init(pos, size, title, btn1Text, btn2Text);
	msgBox->setMessageText(msg, ScrollAction::TOP);
	
	return msgBox;
}

MessageDialog::~MessageDialog(){
	delete m_scrollText;
}

void MessageDialog::setMessageText(const string &text, ScrollAction action) {
	m_scrollText->setText(text, action);
}

// =====================================================
// class InputBox
// =====================================================

InputBox::InputBox(Container *parent)
		: TextBox(parent) {
}

//InputBox::InputBox(Container *parent, Vec2i pos, Vec2i size)
//		: TextBox(parent, pos, size){
//}

bool InputBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::ESCAPE:
			Escaped(this);
			return true;
	}
	return TextBox::keyDown(key);
}

// =====================================================
//  class InputDialog
// =====================================================

InputDialog::InputDialog(WidgetWindow* window)
		: BasicDialog(window) {
	Anchors cAnchors;
	cAnchors.setCentre(true);

	CellStrip *panel = new CellStrip(this, Orientation::VERTICAL, 2);
	setContent(panel);
	m_label = new StaticText(panel);
	m_label->setCell(0);
	m_label->setAnchors(cAnchors);
	m_label->setText("");
	m_inputBox = new InputBox(panel);
	m_inputBox->setCell(1);
	m_inputBox->setAnchors(cAnchors);
	m_inputBox->setText("");
	m_inputBox->InputEntered.connect(this, &InputDialog::onInputEntered);
	m_inputBox->Escaped.connect(this, &InputDialog::onEscaped);
}

InputDialog* InputDialog::showDialog(Vec2i pos, Vec2i size, const string &title, const string &msg, 
							   const string &btn1Text, const string &btn2Text) {
	InputDialog* dlg = new InputDialog(&g_widgetWindow);
	g_widgetWindow.setFloatingWidget(dlg, true);
	dlg->init(pos, size, title, btn1Text, btn2Text);
	dlg->setMessageText(msg);
	dlg->m_inputBox->gainFocus();
	dlg->m_inputBox->Escaped.connect(dlg, &InputDialog::onEscaped);
	return dlg;
}

void InputDialog::onInputEntered(Widget*) {
	if (!m_inputBox->getText().empty()) {
		Button1Clicked(this);
	}
}

void InputDialog::setMessageText(const string &text) {
	m_label->setText(text);
}

}}
