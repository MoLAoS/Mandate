// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2011	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "keymap_widget.h"
#include "user_interface.h"
#include "program.h"

namespace Glest { namespace Gui {

using Main::Program;

// =====================================================
// 	class HotKeyInputBox
// =====================================================

HotKeyInputBox::HotKeyInputBox(Container *parent)
		: InputBox(parent), m_hotKey(KeyCode::NONE, 0) {
}

bool HotKeyInputBox::keyDown(Key key) {
	if (!key.isModifier()) {
		cout << "HotKeyInputBox::keyDown( " << KeyCodeNames[key.getCode()] << " )" << endl;
	}
	return true;
}

bool HotKeyInputBox::keyUp(Key key) {
	if (!key.isModifier() && key.getCode() != KeyCode::NONE) {
		cout << "HotKeyInputBox::keyUp( " << KeyCodeNames[key.getCode()] << " )" << endl;
		string txt;
		int flags = 0;
		if (g_program.getInput().isShiftDown()) {
			flags |= ModKeys::SHIFT;
			txt += "Shift+";
		}
		if (g_program.getInput().isCtrlDown()) {
			flags |= ModKeys::CTRL;
			txt += "Ctrl+";
		}
		if (g_program.getInput().isAltDown()) {
			flags |= ModKeys::ALT;
			txt += "Alt+";
		}
		if (g_program.getInput().isMetaDown()) {
			flags |= ModKeys::META;
			txt += "Meta+";
		}
		HotKey hk = HotKey(key.getCode(), flags);
		txt += formatString(KeyCodeNames[hk.getKey()]);
		if (m_hotKey != hk) {
			m_hotKey = hk;
			setText(txt);
			Changed(this);
		}
	}
	return true;
}

bool HotKeyInputBox::keyPress(char c) {
	return true;
}

void HotKeyInputBox::setHotKey(HotKey hotKey) {
	m_hotKey = hotKey;
	string txt;
	if (m_hotKey.getMod() & ModKeys::SHIFT) {
		txt += "Shift+";
	}
	if (m_hotKey.getMod() & ModKeys::CTRL) {
		txt += "Ctrl+";
	}
	if (m_hotKey.getMod() & ModKeys::ALT) {
		txt += "Alt+";
	}
	if (m_hotKey.getMod() & ModKeys::META) {
		txt += "Meta+";
	}
	txt += formatString(KeyCodeNames[m_hotKey.getKey()]);
	setText(txt);
}

// =====================================================
// 	class KeyEntryWidget
// =====================================================

KeyEntryWidget::KeyEntryWidget(Container *parent, Keymap &keymap, UserCommand uc) 
		: CellStrip(parent, Orientation::HORIZONTAL, 5)
		, m_keymap(keymap)
		, m_userCommand(uc)
		, m_msgBox(0) {
	setSizeHint(0, SizeHint(30));
	setSizeHint(1, SizeHint(35));
	setSizeHint(2, SizeHint(35));

	m_label = new StaticText(this);
	m_label->setCell(0);
	m_label->setAnchors(Anchors::getFillAnchors());
	m_label->setText(formatString(UserCommandNames[uc]));

	HotKeyAssignment &hka = m_keymap.getAssignment(uc);
	hka.Modified.connect(this, &KeyEntryWidget::onHotKeyAssignmentChanged);

	m_inputBox1 = new HotKeyInputBox(this);
	m_inputBox1->setCell(1);
	m_inputBox1->setAnchors(Anchors::getFillAnchors());
	m_inputBox1->setHotKey(hka.getHotKey1());
	m_inputBox1->Changed.connect(this, &KeyEntryWidget::onHotKeyChanged);

	m_inputBox2 = new HotKeyInputBox(this);
	m_inputBox2->setCell(2);
	m_inputBox2->setAnchors(Anchors::getFillAnchors());
	m_inputBox2->setHotKey(hka.getHotKey2());
	m_inputBox2->Changed.connect(this, &KeyEntryWidget::onHotKeyChanged);

}

void KeyEntryWidget::onHotKeyChanged(Widget *wdgt) {
	HotKeyAssignment &hka = m_keymap.getAssignment(m_userCommand);
	HotKeyInputBox *ib = static_cast<HotKeyInputBox*>(wdgt);
	UserCommand existingAssignment = m_keymap.getCommand(ib->getHotKey());
	
	if (existingAssignment > UserCommand::NONE && existingAssignment != m_userCommand) {

		string msg = "The key combo '" + ib->getHotKey().toString() + "' is currently assigned to '"
			+ formatString(UserCommandNames[existingAssignment]) 
			+ "'. Do you want to replace this assignment?";
		Vec2i sz(600, 400);
		Vec2i pos = (g_metrics.getScreenDims() - sz) / 2;
		m_msgBox = MessageDialog::showDialog(pos, sz, "Change Hotkey?", msg, "Yes (Change)", "No (Cancel)");
		m_msgBox->Button1Clicked.connect(this, &KeyEntryWidget::onConfirmHotKeyChange);
		m_msgBox->Button2Clicked.connect(this, &KeyEntryWidget::onCancelHotKeyChange);
		m_msgBox->Close.connect(this, &KeyEntryWidget::onCancelHotKeyChange);
		if (ib == m_inputBox1) {
			m_firstEntry = true;
			m_prevValue = hka.getHotKey1();
		} else {
			m_firstEntry = false;
			m_prevValue = hka.getHotKey2();
		}
		m_nextValue = ib->getHotKey();

	} else {
		if (ib == m_inputBox1) {
			hka.getHotKey1().init(ib->getText());
		} else {
			hka.getHotKey2().init(ib->getText());
		}
		m_keymap.save();
	}
}

void KeyEntryWidget::onConfirmHotKeyChange(Widget*) {
	// remove old assignment
	assert(m_nextValue.isSet());
	UserCommand oldCommand = m_keymap.getCommand(m_nextValue);
	assert(oldCommand > UserCommand::NONE && oldCommand < UserCommand::COUNT);
	HotKeyAssignment &hkaOld = m_keymap.getAssignment(oldCommand);
	if (hkaOld.getHotKey1() == m_nextValue) {
		hkaOld.setHotKey1(HotKey());
	} else if (hkaOld.getHotKey2() == m_nextValue) {
		hkaOld.setHotKey2(HotKey());
	} else {
		assert(false);
	}

	// assign new hotkey
	HotKeyAssignment &hka = m_keymap.getAssignment(m_userCommand);
	if (m_firstEntry) {
		hka.setHotKey1(m_nextValue);
	} else {
		hka.setHotKey2(m_nextValue);
	}
	g_widgetWindow.removeFloatingWidget(m_msgBox);
	m_msgBox = 0;
	m_prevValue = m_nextValue = HotKey();
}

void KeyEntryWidget::onCancelHotKeyChange(Widget*) {
	if (m_firstEntry) {
		m_inputBox1->setHotKey(m_prevValue);
	} else {
		m_inputBox2->setHotKey(m_prevValue);
	}
	g_widgetWindow.removeFloatingWidget(m_msgBox);
	m_msgBox = 0;
	m_prevValue = m_nextValue = HotKey();
}

void KeyEntryWidget::onHotKeyAssignmentChanged(HotKeyAssignment *assignment) {
	assert(assignment->getUserCommand() == m_userCommand);
	m_inputBox1->setHotKey(assignment->getHotKey1());
	m_inputBox2->setHotKey(assignment->getHotKey2());
}

// =====================================================
//  class KeymapWidget
// =====================================================

KeymapWidget::KeymapWidget(Container *parent)
		: CellStrip(parent, Orientation::HORIZONTAL, 2) {
	CellStrip *leftCol = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP);
	leftCol->setAnchors(Anchors::getFillAnchors());
	leftCol->setCell(0);
	CellStrip *rightCol = new CellStrip(this, Orientation::VERTICAL, Origin::FROM_TOP);
	rightCol->setCell(1);
	rightCol->setAnchors(Anchors::getFillAnchors());

	Keymap &keymap = g_program.getKeymap();
	int ih = g_widgetConfig.getDefaultItemHeight() * 5 / 4;
	int lc = 0;
	int rc = 0;
	for (UserCommand uc(1); uc != UserCommand::COUNT; ++uc) {
		if (uc <= UserCommand::SELECT_IDLE_HARVESTER) {
			KeyEntryWidget *kew = new KeyEntryWidget(leftCol, keymap, uc);
			kew->setAnchors(Anchors::getFillAnchors());
			leftCol->addCells(1);
			leftCol->setSizeHint(lc, SizeHint(-1, ih));
			kew->setCell(lc++);
		} else {
			KeyEntryWidget *kew = new KeyEntryWidget(rightCol, keymap, uc);
			kew->setAnchors(Anchors::getFillAnchors());
			rightCol->addCells(1);
			rightCol->setSizeHint(rc, SizeHint(-1, ih));
			kew->setCell(rc++);
		}
	}
}

}}
