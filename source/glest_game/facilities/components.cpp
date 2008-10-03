// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa,
//				  2008 Jaagup Repän <jrepan@gmail.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "components.h"

#include <cassert>
#include <algorithm>

#include "metrics.h"
#include "core_data.h"
#include "platform_util.h"
#include "window.h"
#include "font.h"
#include "math_util.h"

#include "leak_dumper.h"


using namespace std;
using namespace Shared::Graphics;

namespace Glest{ namespace Game{

// =====================================================
//	class GraphicComponent
// =====================================================

float GraphicComponent::anim= 0.f;
float GraphicComponent::fade= 0.f;
const float GraphicComponent::animSpeed= 0.02f;
const float GraphicComponent::fadeSpeed= 0.01f;

void GraphicComponent::init(int x, int y, int w, int h){
    this->x= x;
    this->y= y;
    this->w= w;
    this->h= h;
    font= CoreData::getInstance().getMenuFontNormal();
}

bool GraphicComponent::mouseMove(int x, int y){
    return
        x > this->x &&
        y > this->y &&
        x < this->x + w &&
        y < this->y + h;
}

bool GraphicComponent::mouseClick(int x, int y){
    return mouseMove(x, y);
}

void GraphicComponent::update(){
	fade+= fadeSpeed;
	anim+= animSpeed;
	if(fade>1.f) fade= 1.f;
	if(anim>1.f) anim= 0.f;
}

void GraphicComponent::resetFade(){
	fade= 0.f;
}

// =====================================================
//	class GraphicLabel
// =====================================================

const int GraphicLabel::defH= 20;
const int GraphicLabel::defW= 70;

void GraphicLabel::init(int x, int y, int w, int h, bool centered){
	GraphicComponent::init(x, y, w, h);
	this->centered= centered;
}

// =====================================================
//	class GraphicButton
// =====================================================

const int GraphicButton::defH= 22;
const int GraphicButton::defW= 90;

void GraphicButton::init(int x, int y, int w, int h){
	GraphicComponent::init(x, y, w, h);
    lighted= false;
}

bool GraphicButton::mouseMove(int x, int y){
    bool b= GraphicComponent::mouseMove(x, y);
    lighted= b;
    return b;
}

// =====================================================
//	class GraphicListBox
// =====================================================

const int GraphicListBox::defH= 22;
const int GraphicListBox::defW= 140;

void GraphicListBox::init(int x, int y, int w, int h){
	GraphicComponent::init(x, y, w, h);

	graphButton1.init(x, y, 22, h);
    graphButton2.init(x+w-22, y, 22, h);
    graphButton1.setText("<");
    graphButton2.setText(">");
    selectedItemIndex=-1;
}

//queryes
void GraphicListBox::pushBackItem(string item){
    items.push_back(item);
    setSelectedItemIndex(0);
}

void GraphicListBox::setItems(const vector<string> &items){
    this->items= items;
    setSelectedItemIndex(0);
}

void GraphicListBox::setSelectedItemIndex(int index){
    assert(index>=0 && index<items.size());
    selectedItemIndex= index;
    setText(getSelectedItem());
}

void GraphicListBox::setSelectedItem(string item){
	vector<string>::iterator iter;

    iter= find(items.begin(), items.end(), item);

	if(iter==items.end()){
        throw runtime_error("Value not found on list box: "+item);
	}

    setSelectedItemIndex(iter-items.begin());

}

bool GraphicListBox::mouseMove(int x, int y){
    return
        graphButton1.mouseMove(x, y) ||
        graphButton2.mouseMove(x, y);
}

bool GraphicListBox::mouseClick(int x, int y){
	if(!items.empty()){
		bool b1= graphButton1.mouseClick(x, y);
		bool b2= graphButton2.mouseClick(x, y);

		if(b1){
			selectedItemIndex--;
			if(selectedItemIndex<0){
				selectedItemIndex=items.size()-1;
			}
		}
		else if(b2){
			selectedItemIndex++;
			if(selectedItemIndex>=items.size()){
				selectedItemIndex=0;
			}
		}
		setText(getSelectedItem());

		return b1 || b2;
	}
	return false;
}

// =====================================================
//	class GraphicMessageBox
// =====================================================

const int GraphicMessageBox::defH= 150;
const int GraphicMessageBox::defW= 300;
// bunch of sloppy crap...
void GraphicMessageBox::init(const string &text, const string &button1Str){
	init();
	GraphicComponent::setText(text);
	layout();
	buttonCount= 1;

	button1.init(x + (w - GraphicButton::defW) / 2, y + 25);
	button1.setText(button1Str);
}

void GraphicMessageBox::init(const string &text, const string &button1Str, const string &button2Str){
	init();
	GraphicComponent::setText(text);
	layout();
	buttonCount = 2;

	button1.init(x + (w - GraphicButton::defW) / 4, y + 25);
	button1.setText(button1Str);

	button2.init(x + 3 * (w - GraphicButton::defW) / 4, y + 25);
	button2.setText(button2Str);
}

void GraphicMessageBox::init() {
	font = CoreData::getInstance().getMenuFontNormal();
}

void GraphicMessageBox::layout() {
	const FontMetrics *fontMetrics = font->getMetrics();
	Vec2f dim = fontMetrics->getTextDiminsions(text);
	w = max(defW, (int)roundf(dim.x * 0.7f + defW / 16.f));
	h = max(defH, (int)roundf(dim.y * 1.75f + defH * 0.5f));

	const Metrics &metrics = Metrics::getInstance();

	x = (metrics.getVirtualW() - w) / 2;
	y = (metrics.getVirtualH() - h) / 2;

	if(buttonCount == 1) {
		button1.init(x + (w - GraphicButton::defW) / 2, y + 25);
	} else if(buttonCount == 2) {
		button1.init(x + (w - GraphicButton::defW) / 4, y + 25);
		button2.init(x + 3 * (w - GraphicButton::defW) / 4, y + 25);
	}
}

bool GraphicMessageBox::mouseMove(int x, int y){
	return button1.mouseMove(x, y) || (buttonCount > 1 && button2.mouseMove(x, y));
}

bool GraphicMessageBox::mouseClick(int x, int y){
	return button1.mouseClick(x, y) || (buttonCount > 1 && button2.mouseClick(x, y));
}

bool GraphicMessageBox::mouseClick(int x, int y, int &clickedButton){
	if(button1.mouseClick(x, y)) {
		clickedButton = 1;
		return true;
	}

	if(buttonCount > 1 && button2.mouseClick(x, y)) {
		clickedButton= 2;
		return true;
	}

	return false;
}

// ===========================================================
// 	class GraphicTextEntry
// ===========================================================


const int GraphicTextEntry::defH= 22;
const int GraphicTextEntry::defW= 90;

void GraphicTextEntry::init(int x, int y, int w, int h) {
	GraphicComponent::init(x, y, w, h);
	activated1 = false;
	activated2 = false;
	text = "";
}

bool GraphicTextEntry::mouseMove(int x, int y) {
	activated1 =
			x > this->x &&
			y > this->y &&
			x < this->x + w &&
			y < this->y + h;
	return activated1;
}

bool GraphicTextEntry::mouseClick(int x, int y) {
	activated2 = mouseMove(x, y);
	return activated2;
}

void GraphicTextEntry::keyDown(char key) {
	if (isActivated()) {
		if ((key >= 'A') && (key <= 'Z')) {			// character
			text += key - 'A' + 'a';
		} else if ((key >= 32) && (key <= 126)) {	// other symbol
			text += key;
		} else if (key == 127) {					// delete
			text = "";
		} else if (key == vkBack) {					// backspace
			text.erase(text.end() - 1);
		}
	}
}

// =====================================================
//	class GraphicTextEntryBox
// =====================================================

const int GraphicTextEntryBox::defH= 150;
const int GraphicTextEntryBox::defW= 300;

void GraphicTextEntryBox::init(const string &button1Str, const string &button2Str,
		const string &title, const string &text, const string &entryText) {

	font= CoreData::getInstance().getMenuFontNormal();

	h= defH;
	w= defW;

	const Metrics &metrics= Metrics::getInstance();

	x= (metrics.getVirtualW()-w)/2;
	y= (metrics.getVirtualH()-h)/2;;

	button1.init(x+(w-GraphicButton::defW)/4, y+25);
	button1.setText(button1Str);

	button2.init(x+3*(w-GraphicButton::defW)/4, y+25);
	button2.setText(button2Str);

	entry.init(x + 100, y+h/2);
	entry.setText(entryText);

	label.init(x, y+h-25, w, GraphicLabel::defH, true);
	label.setText(title);

	setText(text);
}

bool GraphicTextEntryBox::mouseMove(int x, int y) {
	entry.mouseMove(x, y);
	return button1.mouseMove(x, y) || button2.mouseMove(x, y);
}

bool GraphicTextEntryBox::mouseClick(int x, int y) {
	entry.mouseClick(x, y);
	return button1.mouseClick(x, y) || button2.mouseClick(x, y);
}

bool GraphicTextEntryBox::mouseClick(int x, int y, int &clickedButton) {
	bool b1= button1.mouseClick(x, y);
	bool b2= button2.mouseClick(x, y);

	entry.mouseClick(x, y);

	if(b1) {
		clickedButton= 1;
		return true;
	} else if(b2) {
		clickedButton= 2;
		return true;
	}
	return false;
}

void GraphicTextEntryBox::keyDown(char key) {
//	if(key != vkReturn && key != vkReturn) {
		entry.keyDown(key);
//	}
}

}}//end namespace
