// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"

#include "main.h"

#include <ctime>
#include <wx/dir.h>

#include "conversion.h"
#include "FSFactory.hpp"

#include "icons.h"


using std::exception;
using namespace Shared::Util;
using Shared::PhysFS::FSFactory;

namespace MapEditor {

const string MainWindow::versionString = "v1.5.0";
const string MainWindow::winHeader = "Glest Map Editor " + versionString + " - Built: " + __DATE__;

// ===============================================
//	class Global functions
// ===============================================

wxString ToUnicode(const char* str) {
	return wxString(str, wxConvUTF8);
}

wxString ToUnicode(const string& str) {
	return wxString(str.c_str(), wxConvUTF8);
}

// ===============================================
// class MainWindow
// ===============================================

MainWindow::MainWindow()
		: wxFrame(NULL, -1,  ToUnicode(winHeader), wxDefaultPosition, wxSize(800, 600))
		, lastX(0), lastY(0)
		, currentBrush(btHeight)
		, height(0)
		, surface(1)
		, radius(1)
		, object(0)
		, resource(0)
		, startLocation(1)
		, enabledGroup(ctHeight)
		, fileModified(false)
		, program(0) {
	fileName = "New (unsaved) map";

	SetIcon(wxIcon(gae_mapeditor_xpm));

	this->panel = new wxPanel(this, wxID_ANY);

	//gl canvas
	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
	glCanvas = new GlCanvas(this, this->panel, args);

	buildMenuBar();
	buildToolBars();
	buildStatusBar();

	wxBoxSizer *boxsizer = new wxBoxSizer(wxVERTICAL);
	boxsizer->Add(toolbar, 0, wxEXPAND);
	boxsizer->Add(toolbar2, 0, wxEXPAND);
	boxsizer->Add(glCanvas, 1, wxEXPAND);

	panel->SetSizer(boxsizer);
	panel->Layout();

	glCanvas->SetFocus();
}

void MainWindow::buildMenuBar() {
	//menus
	menuBar = new wxMenuBar();

	//file
	menuFile = new wxMenu();
	menuFile->Append(wxID_NEW);
	menuFile->Append(wxID_OPEN);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_SAVE);
	menuFile->Append(wxID_SAVEAS);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	menuBar->Append(menuFile, wxT("&File"));

	//edit
	menuEdit = new wxMenu();
	menuEdit->Append(miEditUndo, wxT("&Undo\tCTRL+Z"));
	menuEdit->Append(miEditRedo, wxT("&Redo\tCTRL+Y"));
	menuEdit->Append(miEditReset, wxT("Rese&t"));
	menuEdit->Append(miEditResetPlayers, wxT("Reset &Players"));
	menuEdit->Append(miEditResize, wxT("Re&size"));
	menuEdit->Append(miEditFlipX, wxT("Flip &X"));
	menuEdit->Append(miEditFlipY, wxT("Flip &Y"));
	menuEdit->Append(miEditRandomizeHeights, wxT("Randomize &Heights"));
	menuEdit->Append(miEditRandomize, wxT("Randomi&ze"));
	menuEdit->Append(miEditSwitchSurfaces, wxT("Switch Su&rfaces"));
	menuEdit->Append(miEditInfo, wxT("&Info"));
	menuEdit->Append(miEditAdvanced, wxT("&Advanced"));
	menuBar->Append(menuEdit, wxT("&Edit"));

	//misc
	menuMisc = new wxMenu();
	menuMisc->Append(miMiscResetZoomAndPos, wxT("&Reset zoom and pos"));
	menuMisc->Append(miMiscAbout, wxT("&About"));
	menuMisc->Append(miMiscHelp, wxT("&Help"));
	menuMisc->Append(miMiscShowMap, wxT("&Show Map\tCTRL+M"));
	menuBar->Append(menuMisc, wxT("&Misc"));

	//brush
	menuBrush = new wxMenu();

	// Glest height brush
	menuBrushHeight = new wxMenu();
	for (int i = 0; i < heightCount; ++i) {
		menuBrushHeight->AppendCheckItem(miBrushHeight + i + 1, ToUnicode(intToStr(i - heightCount / 2)));
	}
	menuBrushHeight->Check(miBrushHeight + (heightCount + 1) / 2, true);
	menuBrush->Append(miBrushHeight, wxT("&Height"), menuBrushHeight);

	enabledGroup = ctHeight;

	// ZombiePirate height brush
	menuBrushGradient = new wxMenu();
	for (int i = 0; i < heightCount; ++i) {
		menuBrushGradient->AppendCheckItem(miBrushGradient + i + 1, ToUnicode(intToStr(i - heightCount / 2)));
	}
	menuBrush->Append(miBrushGradient, wxT("&Gradient"), menuBrushGradient);

	//surface
	menuBrushSurface = new wxMenu();
	menuBrushSurface->AppendCheckItem(miBrushSurface + 1, wxT("&1 - Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 2, wxT("&2 - Secondary Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 3, wxT("&3 - Road"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 4, wxT("&4 - Stone"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 5, wxT("&5 - Ground"));
	menuBrush->Append(miBrushSurface, wxT("&Surface"), menuBrushSurface);

	//objects
	menuBrushObject = new wxMenu();
	menuBrushObject->AppendCheckItem(miBrushObject + 1, wxT("&0 - None (erase)"));
	menuBrushObject->AppendCheckItem(miBrushObject+2, wxT("&1 - Tree (unwalkable/harvestable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+3, wxT("&2 - DeadTree/Cactuses/Thornbush (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+4, wxT("&3 - Stone (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+5, wxT("&4 - Bush/Grass/Fern (walkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+6, wxT("&5 - Water Object/Reed/Papyrus (walkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+7, wxT("&6 - C1 BigTree/DeadTree/OldPalm (unwalkable/not harvestable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+8, wxT("&7 - C2 Hanged/Impaled (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+9, wxT("&8 - C3 Statues (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+10, wxT("&9 - C4 Big Rock (Mountain) (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+11, wxT("10 &- C5 Invisible Blocking Object (unwalkable)"));
	menuBrush->Append(miBrushObject, wxT("&Object"), menuBrushObject);

	//resources
	menuBrushResource = new wxMenu();
	menuBrushResource->AppendCheckItem(miBrushResource + 1, wxT("&0 - None"));
	menuBrushResource->AppendCheckItem(miBrushResource+2, wxT("&1 - gold  (unwalkable)"));
	menuBrushResource->AppendCheckItem(miBrushResource+3, wxT("&2 - stone (unwalkable)"));
	menuBrushResource->AppendCheckItem(miBrushResource+4, wxT("&3 - custom"));
	menuBrushResource->AppendCheckItem(miBrushResource+5, wxT("&4 - custom"));
	menuBrushResource->AppendCheckItem(miBrushResource+6, wxT("&5 - custom"));
	menuBrush->Append(miBrushResource, wxT("&Resource"), menuBrushResource);

	bmStartPos[0] = wxBitmap(brush_players_red);
	bmStartPos[1] = wxBitmap(brush_players_blue);
	bmStartPos[2] = wxBitmap(brush_players_green);
	bmStartPos[3] = wxBitmap(brush_players_yellow);
	bmStartPos[4] = wxBitmap(brush_players_white);
	bmStartPos[5] = wxBitmap(brush_players_cyan);
	bmStartPos[6] = wxBitmap(brush_players_orange);
	bmStartPos[7] = wxBitmap(brush_players_pink);

	//players
	menuBrushStartLocation = new wxMenu();
	miStartPos[0] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 1, wxT("&1 - Player 1"));
	miStartPos[0]->SetBitmap(bmStartPos[0]);
	menuBrushStartLocation->Append(miStartPos[0]);
	miStartPos[1] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 2, wxT("&2 - Player 2"));
	miStartPos[1]->SetBitmap(bmStartPos[1]);
	menuBrushStartLocation->Append(miStartPos[1]);
	miStartPos[2] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 3, wxT("&3 - Player 3"));
	miStartPos[2]->SetBitmap(bmStartPos[2]);
	menuBrushStartLocation->Append(miStartPos[2]);
	miStartPos[3] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 4, wxT("&4 - Player 4"));
	miStartPos[3]->SetBitmap(bmStartPos[3]);
	menuBrushStartLocation->Append(miStartPos[3]);
	miStartPos[4] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 5, wxT("&5 - Player 5"));
	miStartPos[4]->SetBitmap(bmStartPos[4]);
	menuBrushStartLocation->Append(miStartPos[4]);
	miStartPos[5] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 6, wxT("&6 - Player 6"));
	miStartPos[5]->SetBitmap(bmStartPos[5]);
	menuBrushStartLocation->Append(miStartPos[5]);
	miStartPos[6] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 7, wxT("&7 - Player 7"));
	miStartPos[6]->SetBitmap(bmStartPos[6]);
	menuBrushStartLocation->Append(miStartPos[6]);
	miStartPos[7] = new wxMenuItem(menuBrushStartLocation, miBrushStartLocation + 8, wxT("&8 - Player 8"));
	miStartPos[7]->SetBitmap(bmStartPos[7]);
	menuBrushStartLocation->Append(miStartPos[7]);
	menuBrush->Append(miBrushStartLocation, wxT("&Player"), menuBrushStartLocation);
	menuBar->Append(menuBrush, wxT("&Brush"));

	//radius
	menuRadius = new wxMenu();
	for (int i = 1; i <= radiusCount; ++i) {
		menuRadius->AppendCheckItem(miRadius + i, ToUnicode("&" + intToStr(i) + "\tALT+" + intToStr(i)));
	}
	menuRadius->Check(miRadius + 1, true);
	menuBar->Append(menuRadius, wxT("&Radius"));
	SetMenuBar(menuBar);
}

void MainWindow::buildToolBars() {
	toolbar = new wxToolBar(this->panel, wxID_ANY);
	toolbar->AddTool(miEditUndo, _("undo"), wxBitmap(edit_undo), _("Undo"));
	toolbar->AddTool(miEditRedo, _("redo"), wxBitmap(edit_redo), _("Redo"));
	toolbar->AddTool(miEditRandomizeHeights, _("randomizeHeights"), wxBitmap(edit_randomize_heights), _("Randomize Heights"));
	toolbar->AddTool(miEditRandomize, _("randomize"), wxBitmap(edit_randomize), _("Randomize"));
	toolbar->AddTool(miEditSwitchSurfaces, _("switch"), wxBitmap(edit_switch_surfaces), _("Switch Surfaces"));
	toolbar->AddSeparator();
	toolbar->AddTool(miBrushSurface + 1, _("brush_grass1"), wxBitmap(brush_surface_grass1), _("Grass"));
	toolbar->AddTool(miBrushSurface + 2, _("brush_grass2"), wxBitmap(brush_surface_grass2), _("Secondary Grass"));
	toolbar->AddTool(miBrushSurface + 3, _("brush_road"), wxBitmap(brush_surface_road), _("Road"));
	toolbar->AddTool(miBrushSurface + 4, _("brush_stone"), wxBitmap(brush_surface_stone), _("Stone"));
	toolbar->AddTool(miBrushSurface + 5, _("brush_custom"), wxBitmap(brush_surface_custom), _("Ground"));
	toolbar->AddSeparator();
	toolbar->AddTool(miBrushResource + 2, _("resource1"), wxBitmap(brush_resource_1_gold), _("gold  (unwalkable)"));
	toolbar->AddTool(miBrushResource + 3, _("resource2"), wxBitmap(brush_resource_2_stone), _("stone (unwalkable)"));
	toolbar->AddTool(miBrushResource + 4, _("resource3"), wxBitmap(brush_resource_3), _("custom3"));
	toolbar->AddTool(miBrushResource + 5, _("resource4"), wxBitmap(brush_resource_4), _("custom4"));
	toolbar->AddTool(miBrushResource + 6, _("resource5"), wxBitmap(brush_resource_5), _("custom5"));
	toolbar->AddSeparator();
	toolbar->AddTool(miBrushObject + 1, _("brush_none"), wxBitmap(brush_none), _("None (erase)"));
	toolbar->AddTool(miBrushObject + 2, _("brush_tree"), wxBitmap(brush_object_tree), _("Tree (unwalkable/harvestable)"));
	toolbar->AddTool(miBrushObject + 3, _("brush_dead_tree"), wxBitmap(brush_object_dead_tree), _("DeadTree/Cactuses/Thornbush (unwalkable)"));
	toolbar->AddTool(miBrushObject + 4, _("brush_stone"), wxBitmap(brush_object_stone), _("Stone (unwalkable)"));
	toolbar->AddTool(miBrushObject + 5, _("brush_bush"), wxBitmap(brush_object_bush), _("Bush/Grass/Fern (walkable)"));
	toolbar->AddTool(miBrushObject + 6, _("brush_water"), wxBitmap(brush_object_water_object), _("Water Object/Reed/Papyrus (walkable)"));
	toolbar->AddTool(miBrushObject + 7, _("brush_c1_bigtree"), wxBitmap(brush_object_c1_bigtree), _("C1 BigTree/DeadTree/OldPalm (unwalkable/not harvestable)"));
	toolbar->AddTool(miBrushObject + 8, _("brush_c2_hanged"), wxBitmap(brush_object_c2_hanged), _("C2 Hanged/Impaled (unwalkable)"));
	toolbar->AddTool(miBrushObject + 9, _("brush_c3_statue"), wxBitmap(brush_object_c3_statue), _("C3, Statues (unwalkable))"));
	toolbar->AddTool(miBrushObject +10, _("brush_c4_bigrock"), wxBitmap(brush_object_c4_bigrock), _("Big Rock (Mountain) (unwalkable)"));
	toolbar->AddTool(miBrushObject +11, _("brush_c5_blocking"), wxBitmap(brush_object_c5_blocking), _("Invisible Blocking Object (unwalkable)"));
	toolbar->AddSeparator();
	toolbar->AddTool(toolPlayer, _("brush_player"), wxBitmap(brush_players_player),  _("Player start position"));
	toolbar->Realize();

	toolbar2 = new wxToolBar(this->panel, wxID_ANY);
	toolbar2->AddTool(miBrushGradient + 1, _("brush_gradient_n5"), wxBitmap(brush_gradient_n5));
	toolbar2->AddTool(miBrushGradient + 2, _("brush_gradient_n4"), wxBitmap(brush_gradient_n4));
	toolbar2->AddTool(miBrushGradient + 3, _("brush_gradient_n3"), wxBitmap(brush_gradient_n3));
	toolbar2->AddTool(miBrushGradient + 4, _("brush_gradient_n2"), wxBitmap(brush_gradient_n2));
	toolbar2->AddTool(miBrushGradient + 5, _("brush_gradient_n1"), wxBitmap(brush_gradient_n1));
	toolbar2->AddTool(miBrushGradient + 6, _("brush_gradient_0"), wxBitmap(brush_gradient_0));
	toolbar2->AddTool(miBrushGradient + 7, _("brush_gradient_p1"), wxBitmap(brush_gradient_p1));
	toolbar2->AddTool(miBrushGradient + 8, _("brush_gradient_p2"), wxBitmap(brush_gradient_p2));
	toolbar2->AddTool(miBrushGradient + 9, _("brush_gradient_p3"), wxBitmap(brush_gradient_p3));
	toolbar2->AddTool(miBrushGradient +10, _("brush_gradient_p4"), wxBitmap(brush_gradient_p4));
	toolbar2->AddTool(miBrushGradient +11, _("brush_gradient_p5"), wxBitmap(brush_gradient_p5));
	toolbar2->AddSeparator();
	toolbar2->AddTool(miBrushHeight + 1, _("brush_height_n5"), wxBitmap(brush_height_n5));
	toolbar2->AddTool(miBrushHeight + 2, _("brush_height_n4"), wxBitmap(brush_height_n4));
	toolbar2->AddTool(miBrushHeight + 3, _("brush_height_n3"), wxBitmap(brush_height_n3));
	toolbar2->AddTool(miBrushHeight + 4, _("brush_height_n2"), wxBitmap(brush_height_n2));
	toolbar2->AddTool(miBrushHeight + 5, _("brush_height_n1"), wxBitmap(brush_height_n1));
	toolbar2->AddTool(miBrushHeight + 6, _("brush_height_0"), wxBitmap(brush_height_0));
	toolbar2->AddTool(miBrushHeight + 7, _("brush_height_p1"), wxBitmap(brush_height_p1));
	toolbar2->AddTool(miBrushHeight + 8, _("brush_height_p2"), wxBitmap(brush_height_p2));
	toolbar2->AddTool(miBrushHeight + 9, _("brush_height_p3"), wxBitmap(brush_height_p3));
	toolbar2->AddTool(miBrushHeight +10, _("brush_height_p4"), wxBitmap(brush_height_p4));
	toolbar2->AddTool(miBrushHeight +11, _("brush_height_p5"), wxBitmap(brush_height_p5));
	toolbar2->AddSeparator();
	toolbar2->AddTool(miRadius + 1, _("radius1"), wxBitmap(radius_1), _("1 (1x1)"));
	toolbar2->AddTool(miRadius + 2, _("radius2"), wxBitmap(radius_2), _("2 (3x3)"));
	toolbar2->AddTool(miRadius + 3, _("radius3"), wxBitmap(radius_3), _("3 (5x5)"));
	toolbar2->AddTool(miRadius + 4, _("radius4"), wxBitmap(radius_4), _("4 (7x7)"));
	toolbar2->AddTool(miRadius + 5, _("radius5"), wxBitmap(radius_5), _("5 (9x9)"));
	toolbar2->AddTool(miRadius + 6, _("radius6"), wxBitmap(radius_6), _("6 (11x11)"));
	toolbar2->AddTool(miRadius + 7, _("radius7"), wxBitmap(radius_7), _("7 (13x13)"));
	toolbar2->AddTool(miRadius + 8, _("radius8"), wxBitmap(radius_8), _("8 (15x15)"));
	toolbar2->AddTool(miRadius + 9, _("radius9"), wxBitmap(radius_9), _("9 (17x17)"));
	toolbar2->Realize();
}


void MainWindow::buildStatusBar() {
	int status_widths[StatusItems::COUNT] = {
		10, // empty
		-2, // File name
		-1, // File type
		-2, // Current Object
		-2, // Brush Type
		-2, // Brush 'Value'
		-1, // Brush Radius
	};
	CreateStatusBar(StatusItems::COUNT);
	GetStatusBar()->SetStatusWidths(StatusItems::COUNT, status_widths);

	SetStatusText(wxT("File: ") + ToUnicode(fileName), StatusItems::FILE_NAME);
	SetStatusText(wxT(".gbm"), StatusItems::FILE_TYPE);
	SetStatusText(wxT("Object: None (Erase)"), StatusItems::CURR_OBJECT);
	SetStatusText(wxT("Brush: Height"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: 0"), StatusItems::BRUSH_VALUE);
	SetStatusText(wxT("Radius: 1"), StatusItems::BRUSH_RADIUS);
}

void MainWindow::onToolPlayer(wxCommandEvent& event){
	PopupMenu(menuBrushStartLocation);
}

void MainWindow::init(string fname, wxString glest) {
	glCanvas->SetCurrent();
	program = new Program(glCanvas->GetClientSize().x, glCanvas->GetClientSize().y);

	fileName = "New (unsaved) Map";
	if (!fname.empty() && fileExists(fname)) {
		program->loadMap(fname);
		currentFile = fname;
		fileName = cutLastExt(basename(fname));
	}
	this->glest = glest;
	
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
	setDirty(false);
	setExtension();
	centreMap();
	setFactionCount();
}

void MainWindow::centreMap() {
	const wxSize canvasSize = glCanvas->GetClientSize();
	const int	mw = program->getMap()->getW(),
				mh = program->getMap()->getH();

	int cellSize = std::min(canvasSize.x / mw, canvasSize.y / mh);

	int pxWidth = cellSize * mw;
	int pxHeight = cellSize * mh;
	
	int offsetX, offsetY;
	if (pxWidth < canvasSize.x) {
		offsetX = (canvasSize.x - pxWidth) / 2;
	} else {
		offsetX = 0;
	}
	if (pxHeight < canvasSize.y) {
		offsetY = (canvasSize.y - pxHeight) / 2;
	} else {
		offsetY = 0;
	}
	program->resetOffset();
	program->setOffset(offsetX, offsetY);
	program->setCellSize(cellSize);
}

void MainWindow::setFactionCount() {
	size_t curr_n = menuBrushStartLocation->GetMenuItemCount();
	size_t map_n = program->getMap()->getMaxFactions();
	if (curr_n > map_n) {
		for (size_t i = map_n; i < curr_n; ++i) {
			menuBrushStartLocation->Remove(miStartPos[i]);
		}
	} else if (curr_n < map_n) {
		for (size_t i = curr_n; i < map_n; ++i) {
			miStartPos[i]->SetBitmap(bmStartPos[i]); // hack for apparent wx bug @todo investigate
			menuBrushStartLocation->Append(miStartPos[i]);
		}
	}
}

/** @return true to continue */
bool MainWindow::checkChanges() {
	if (this->fileModified) {
		int answer = wxMessageBox(_("There are unsaved modifications. Continue anyway?"), _("Discard changes?"),
			wxYES_NO|wxICON_QUESTION, this);
		return (answer == wxYES);
	}
	return true;
}

void MainWindow::onClose(wxCloseEvent &event) {
	if (checkChanges()) {
		delete this;
	}
}

MainWindow::~MainWindow() {
	delete program;
	program = NULL;
	
	delete glCanvas;
	glCanvas = NULL;

	// Menu items are removed from menuBrushStartLocation if players
	// are less than 8, therefore the pointers are no longer owned.
	// To fix manually delete removed items.
	size_t curr_n = menuBrushStartLocation->GetMenuItemCount();
	for (size_t i = curr_n; i < startLocationCount; ++i) {
		delete miStartPos[i];
		miStartPos[i] = NULL;
	}
}

void MainWindow::setDirty(bool val) {
	//Refresh();
	if (fileModified && val) {
		return;
	}
	fileModified = val;
	if (fileModified) {
		SetStatusText(wxT("File: ") + ToUnicode(fileName) + wxT("*"), StatusItems::FILE_NAME);
	} else {
		SetStatusText(wxT("File: ") + ToUnicode(fileName), StatusItems::FILE_NAME);
	}
}

void MainWindow::setExtension() {
	if (currentFile.empty()) {
		return;
	}
	string extnsn = ext(currentFile);
	if (extnsn == "gbm" || extnsn == "mgm") {
		currentFile = cutLastExt(currentFile);
	}
	if (Program::getMap()->getMaxFactions() <= 4) {
		SetStatusText(wxT(".gbm"), StatusItems::FILE_TYPE);
		currentFile += ".gbm";
	} else {
		SetStatusText(wxT(".mgm"), StatusItems::FILE_TYPE);
		currentFile += ".mgm";
	}
}

// WxGLCanvas::Refresh() is not working on windows, possibly because
// its in a wxPanel... http://wiki.wxwidgets.org/WxGLCanvas
// though none of the work-arounds worked for me.
#ifdef WIN32
#	define REFRESH() wxPaintEvent ev;glCanvas->onPaint(ev);
#else
#	define REFRESH() glCanvas->Refresh();
#endif

void MainWindow::onMouseDown(wxMouseEvent &event, int x, int y) {
	if (event.LeftIsDown()) {
		program->setUndoPoint(enabledGroup);
		program->setRefAlt(x, y);
		change(x, y);
		if (!isDirty()) {
			setDirty(true);
		}
		REFRESH();
	}
	event.Skip();
}

void MainWindow::onMouseMove(wxMouseEvent &event, int x, int y) {
	bool repaint = false;
	int dif;
	if (event.LeftIsDown()) {
		change(x, y);
		repaint = true;
	} else if (event.MiddleIsDown()) {
		dif = (y - lastY);
		if (dif != 0) {
			program->incCellSize(dif / abs(dif));
			repaint = true;
		}
	} else if (event.RightIsDown()) {
		program->setOffset(x - lastX, y - lastY);
		repaint = true;
	} else {
		int currResource = program->getResource(x, y);
		if (currResource > 0) {
			SetStatusText(wxT("Resource: ") + ToUnicode(resource_descs[currResource]), StatusItems::CURR_OBJECT);
			resourceUnderMouse = currResource;
			objectUnderMouse = 0;
		} else {
			int currObject = program->getObject(x, y);
			SetStatusText(wxT("Object: ") + ToUnicode(object_descs[currObject]), StatusItems::CURR_OBJECT);
			resourceUnderMouse = 0;
			objectUnderMouse = currObject;				
		}
	}
	lastX = x;
	lastY = y;

	if (repaint) {
		REFRESH();
	}
	event.Skip();
}

void MainWindow::onMousewheelRotation(wxMouseEvent &event) {
	int i = event.GetWheelRotation();
	program->incCellSize(i / abs(i));
	REFRESH();
	event.Skip();
}

void MainWindow::onMenuFileNew(wxCommandEvent &event){
	if (checkChanges()) {
		delete program;
		this->init("", this->glest);
	}
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event) {
	if (checkChanges()) {
		wxFileDialog fileDialog(this);
		fileDialog.SetWildcard(wxT("Glest Map (*.gbm)|*.gbm|Mega Map (*.mgm)|*.mgm"));
		if (fileDialog.ShowModal() == wxID_OK) {
			currentFile = fileDialog.GetPath().ToAscii();
			program->loadMap(currentFile);
			fileName = cutLastExt(basename(currentFile));
			setExtension();
			SetTitle(ToUnicode(winHeader + "; " + currentFile));
			centreMap();
			setDirty(false);
			setFactionCount();
		}
	}
}

void MainWindow::onMenuFileSave(wxCommandEvent &event) {
	if (currentFile.empty()) {
		wxCommandEvent ev;
		onMenuFileSaveAs(ev);
	} else {
		setExtension();
		program->saveMap(currentFile);
		setDirty(false);
	}
}

void MainWindow::onMenuFileSaveAs(wxCommandEvent &event) {
	wxFileDialog fileDialog(this, wxT("Select file"), wxT(""), wxT(""), wxT("*.gbm|*.mgm"), wxSAVE);
	fileDialog.SetWildcard(wxT("Glest Map (*.gbm)|*.gbm|Mega Map (*.mgm)|*.mgm"));
	if (fileDialog.ShowModal() == wxID_OK) {
		currentFile = fileDialog.GetPath().ToAscii();
		setExtension();
		program->saveMap(currentFile);
		fileName = cutLastExt(basename(currentFile));
		setDirty(false);
	}
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
}

void MainWindow::onMenuFileExit(wxCommandEvent &event) {
	Close();
}

void MainWindow::onMenuEditUndo(wxCommandEvent &event) {
	std::cout << "Undo Pressed" << std::endl;
	if (program->undo()) {
		setDirty();
		REFRESH();
	}
}

void MainWindow::onMenuEditRedo(wxCommandEvent &event) {
	std::cout << "Redo Pressed" << std::endl;
	if (program->redo()) {
		setDirty();
		REFRESH();
	}
}

void MainWindow::onMenuEditReset(wxCommandEvent &event) {
	program->setUndoPoint(ctAll);
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Altitude", "10");
	simpleDialog.addValue("Surface", "1");
	simpleDialog.addValue("Width", "64");
	simpleDialog.addValue("Height", "64");
	simpleDialog.show();
	
	try {
		program->reset(
			Conversion::strToInt(simpleDialog.getValue("Width")),
			Conversion::strToInt(simpleDialog.getValue("Height")),
			Conversion::strToInt(simpleDialog.getValue("Altitude")),
			Conversion::strToInt(simpleDialog.getValue("Surface")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	currentFile = "";
	fileName = "New (unsaved) map";

	centreMap();
	setDirty(false);
}

void MainWindow::onMenuEditResetPlayers(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Factions", intToStr(program->getMap()->getMaxFactions()));
	simpleDialog.show();

	try {
		program->resetFactions(Conversion::strToInt(simpleDialog.getValue("Factions")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
	setExtension();
	setFactionCount();
}

void MainWindow::onMenuEditResize(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Altitude", "10");
	simpleDialog.addValue("Surface", "1");
	simpleDialog.addValue("Height", "64");
	simpleDialog.addValue("Width", "64");
	simpleDialog.show();

	try {
		program->resize(
			Conversion::strToInt(simpleDialog.getValue("Height")),
			Conversion::strToInt(simpleDialog.getValue("Width")),
			Conversion::strToInt(simpleDialog.getValue("Altitude")),
			Conversion::strToInt(simpleDialog.getValue("Surface")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	centreMap();
	setDirty();
}

void MainWindow::onMenuEditFlipX(wxCommandEvent &event) {
	program->flipX();
	setDirty();
}

void MainWindow::onMenuEditFlipY(wxCommandEvent &event) {
	program->flipY();
	setDirty();
}

void MainWindow::onMenuEditRandomizeHeights(wxCommandEvent &event) {
	program->randomizeMapHeights();
	centreMap();
	setDirty();
}

void MainWindow::onMenuEditRandomize(wxCommandEvent &event) {
	program->randomizeMap();
	centreMap();
	setDirty();
	setFactionCount();
}

void MainWindow::onMenuEditSwitchSurfaces(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Surface1", "1");
	simpleDialog.addValue("Surface2", "2");
	simpleDialog.show();

	try {
		program->switchMapSurfaces(
			Conversion::strToInt(simpleDialog.getValue("Surface1")),
			Conversion::strToInt(simpleDialog.getValue("Surface2")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuEditInfo(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Title", program->getMap()->getTitle());
	simpleDialog.addValue("Desc", program->getMap()->getDesc());
	simpleDialog.addValue("Author", program->getMap()->getAuthor());

	simpleDialog.show();

	if (program->setMapTitle(simpleDialog.getValue("Title"))
	|| program->setMapDesc(simpleDialog.getValue("Desc"))
	|| program->setMapAuthor(simpleDialog.getValue("Author"))) {
		if (!isDirty()) {
			setDirty(true);
		}
	}
}

void MainWindow::onMenuEditAdvanced(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Height Factor", intToStr(program->getMap()->getHeightFactor()));
	simpleDialog.addValue("Water Level", intToStr(program->getMap()->getWaterLevel()));

	simpleDialog.show();

	try {
		program->setMapAdvanced(
			Conversion::strToInt(simpleDialog.getValue("Height Factor")),
			Conversion::strToInt(simpleDialog.getValue("Water Level")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuMiscResetZoomAndPos(wxCommandEvent &event) {
	program->resetOffset();
}

void MainWindow::onMenuMiscAbout(wxCommandEvent &event) {
	wxMessageDialog(
		NULL,
		wxT("Glest Map Editor - Copyright 2004 The Glest Team\n(with improvements by others, 2010)."),
		wxT("About")).ShowModal();
}

void MainWindow::onMenuMiscHelp(wxCommandEvent &event) {
	wxMessageDialog(
		NULL,
		wxT("Left mouse click: draw\nRight mouse drag: move\nCenter mouse drag: zoom"),
		wxT("Help")).ShowModal();
}

void MainWindow::onMenuMiscShowMap(wxCommandEvent& event){
	if(this->fileModified || this->currentFile.empty()){
		wxMessageBox(_("You need to save first!"), _("Unsaved"), wxOK|wxICON_ERROR, this);
		return;
	}

	// find glest
	if(glest.empty()){
		wxPathList pathlist;
		pathlist.AddEnvList(_("PATH"));
		glest = pathlist.FindAbsoluteValidPath(_("glestadv"));
		if(glest.empty()){
			// not found in PATH -> search recursively in current directory
			glest = wxDir::FindFirst(_("."), _("glestadv*"));
			if(glest.empty()){
				wxMessageBox(_("Couldn't find glestadv!"), _("Error"), wxOK|wxICON_ERROR, this);
				return;
			}
		}
	}

	wxArrayString output, arrstr;
	wxString command = glest + _(" -list-tilesets");
	cout << command.char_str() << endl;
	wxExecute(command, output);

	for(wxArrayString::const_iterator it=output.begin(); it!=output.end(); ++it){
		if(it->at(0)=='~'){  // only rows beginning with ~ are relevant
			arrstr.Add(it->SubString(1, it->Length()-1));
		}
	}
	if(arrstr.empty()){
		wxMessageBox(_("No tilesets found."), _("Error"), wxOK|wxICON_ERROR, this);
		return;
	}

	wxSingleChoiceDialog dlg(this, _("select tileset"), _("tileset"), arrstr);
	if(dlg.ShowModal()==wxID_OK){
		wxString tileset = arrstr[dlg.GetSelection()];
		
		// leading / stands for absolute path, disables physfs for reading
		command = glest + ToUnicode(" -loadmap \"/"+currentFile+"\" ") + tileset;
		cout << command.char_str() << endl;
		wxExecute(command, wxEXEC_SYNC);
	}
}

void MainWindow::onMenuBrushHeight(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushHeight->Check(e.GetId(), true);
	height = e.GetId() - miBrushHeight - heightCount / 2 - 1;
	enabledGroup = ctHeight;
	currentBrush = btHeight;
	SetStatusText(wxT("Brush: Height"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(height)), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushGradient(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushGradient->Check(e.GetId(), true);
	height = e.GetId() - miBrushGradient - heightCount / 2 - 1;
	enabledGroup = ctGradient;
	currentBrush = btGradient;
	SetStatusText(wxT("Brush: Gradient"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(height)), StatusItems::BRUSH_VALUE);
}


void MainWindow::onMenuBrushSurface(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushSurface->Check(e.GetId(), true);
	surface = e.GetId() - miBrushSurface;
	enabledGroup = ctSurface;
	currentBrush = btSurface;
	SetStatusText(wxT("Brush: Surface"), StatusItems::BRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(surface)) + wxT(" ")
		+ ToUnicode(surface_descs[surface - 1]), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushObject(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushObject->Check(e.GetId(), true);
	object = e.GetId() - miBrushObject - 1;
	enabledGroup = ctObject;
	currentBrush = btObject;
	SetStatusText(wxT("Brush: Object"), StatusItems::BRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(object)) + wxT(" ")
		+ ToUnicode(object_descs[object]), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushResource(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushResource->Check(e.GetId(), true);
	resource = e.GetId() - miBrushResource - 1;
	enabledGroup = ctResource;
	currentBrush = btResource;
	SetStatusText(wxT("Brush: Resource"), StatusItems::BRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(resource)) + wxT(" ")
		+ ToUnicode(resource_descs[resource]), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushStartLocation(wxCommandEvent &e) {
	uncheckBrush();
	//menuBrushStartLocation->Check(e.GetId(), true);
	startLocation = e.GetId() - miBrushStartLocation;
	enabledGroup = ctLocation;
	currentBrush = btStartLocation;
	SetStatusText(wxT("Brush: Start Locations"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(startLocation)), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuRadius(wxCommandEvent &e) {
	uncheckRadius();
	menuRadius->Check(e.GetId(), true);
	radius = e.GetId() - miRadius;
	SetStatusText(wxT("Radius: ") + ToUnicode(intToStr(radius)), StatusItems::BRUSH_RADIUS);
}

void MainWindow::change(int x, int y) {
	switch (enabledGroup) {
	case ctHeight:
		program->glestChangeMapHeight(x, y, height, radius);
		break;
	case ctSurface:
		program->changeMapSurface(x, y, surface, radius);
		break;
	case ctObject:
		program->changeMapObject(x, y, object, radius);
		break;
	case ctResource:
		program->changeMapResource(x, y, resource, radius);
		break;
	case ctLocation:
		program->changeStartLocation(x, y, startLocation - 1);
		break;
	case ctGradient:
		program->pirateChangeMapHeight(x, y, height, radius);
		break;
	}
}

void MainWindow::uncheckBrush() {
	for (int i = 0; i < heightCount; ++i) {
		menuBrushHeight->Check(miBrushHeight + i + 1, false);
	}
	for (int i = 0; i < heightCount; ++i) {
		menuBrushGradient->Check(miBrushGradient + i + 1, false);
	}
	for (int i = 0; i < surfaceCount; ++i) {
		menuBrushSurface->Check(miBrushSurface + i + 1, false);
	}
	for (int i = 0; i < objectCount; ++i) {
		menuBrushObject->Check(miBrushObject + i + 1, false);
	}
	for (int i = 0; i < resourceCount; ++i) {
		menuBrushResource->Check(miBrushResource + i + 1, false);
	}
	///@bug menuBrushStartLocation doesn't have checkable items - hailstone 31Jan2011
	/*
	for (int i = 0; i < startLocationCount; ++i) {
		menuBrushStartLocation->Check(miBrushStartLocation + i + 1, false);
	}
	*/
}

void MainWindow::uncheckRadius() {
	for (int i = 1; i <= radiusCount; ++i) {
		menuRadius->Check(miRadius + i, false);
	}
}

void MainWindow::onKeyDown(wxKeyEvent &e) {
	if (currentBrush == btHeight || currentBrush == btGradient) { // 'height' brush
		if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '5') {
			height = e.GetKeyCode() - 48; // '0'-'5' == 0-5
			if (e.GetModifiers() == wxMOD_CONTROL) { // Ctrl means negative
				height  = -height ;
			}
			int id_offset = heightCount / 2 + height + 1;
			if (currentBrush == btHeight) {
				wxCommandEvent evt(wxEVT_NULL, miBrushHeight + id_offset);
				onMenuBrushHeight(evt);
			} else {
				wxCommandEvent evt(wxEVT_NULL, miBrushGradient + id_offset);
				onMenuBrushGradient(evt);
			}
			return;
		}
	}
	if (currentBrush == btSurface) { // surface texture
		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '5') {
			surface = e.GetKeyCode() - 48; // '1'-'5' == 1-5
			wxCommandEvent evt(wxEVT_NULL, miBrushSurface + surface);
			onMenuBrushSurface(evt);
			return;
		}
	}
	if (currentBrush == btObject) {
		bool valid = true;
		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '9') {
			object = e.GetKeyCode() - 48; // '1'-'9' == 1-9
		} else if (e.GetKeyCode() == '0') { // '0' == 10
			object = 10;
		} else if (e.GetKeyCode() == '-') {	// '-' == 0
			object = 0;
		} else {
			valid = false;
		}
		if (valid) {
			wxCommandEvent evt(wxEVT_NULL, miBrushObject + object + 1);
			onMenuBrushObject(evt);
			return;
		}
	}
	if (currentBrush == btResource) {
		if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '5') {
			resource = e.GetKeyCode() - 48;	// '0'-'5' == 0-5
			wxCommandEvent evt(wxEVT_NULL, miBrushResource + resource + 1);
			onMenuBrushResource(evt);
			return;
		}
	}
	if (currentBrush == btStartLocation) {
		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '8') {
			startLocation = e.GetKeyCode() - 48; // '1'-'8' == 0-7
			if (startLocation <= program->getMap()->getMaxFactions()) {
				wxCommandEvent evt(wxEVT_NULL, miBrushStartLocation + startLocation);
				onMenuBrushStartLocation(evt);
				return;
			}
		}
	}
	if (e.GetKeyCode() == 'H') {
		wxCommandEvent evt(wxEVT_NULL, miBrushHeight + height + heightCount / 2 + 1);
		onMenuBrushHeight(evt);
	} else if (e.GetKeyCode() == ' ') {	
		if (resourceUnderMouse != 0) {
			wxCommandEvent evt(wxEVT_NULL, miBrushResource + resourceUnderMouse + 1);
			onMenuBrushResource(evt);
		} else {
			wxCommandEvent evt(wxEVT_NULL, miBrushObject + objectUnderMouse + 1);
			onMenuBrushObject(evt);
		}
	} else if (e.GetKeyCode() == 'G') {
		wxCommandEvent evt(wxEVT_NULL, miBrushGradient + height + heightCount / 2 + 1);
		onMenuBrushGradient(evt);
	} else if (e.GetKeyCode() == 'S') {
		wxCommandEvent evt(wxEVT_NULL, miBrushSurface + surface);
		onMenuBrushSurface(evt);
	} else if (e.GetKeyCode() == 'O') {
		wxCommandEvent evt(wxEVT_NULL, miBrushObject + object + 1);
		onMenuBrushObject(evt);
	} else if (e.GetKeyCode() == 'R') {
		wxCommandEvent evt(wxEVT_NULL, miBrushResource + resource + 1);
		onMenuBrushResource(evt);
	} else if (e.GetKeyCode() == 'L') {
		wxCommandEvent evt(wxEVT_NULL, miBrushStartLocation + startLocation + 1);
		onMenuBrushStartLocation(evt);
	} else {
		e.Skip();
	}
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)

	EVT_CLOSE(MainWindow::onClose)

	// these are 'handled' by GlCanvas and funneled to these handlers
	//EVT_LEFT_DOWN(MainWindow::onMouseDown)
	//EVT_MOTION(MainWindow::onMouseMove)
	//EVT_KEY_DOWN(MainWindow::onKeyDown)

	EVT_MENU(wxID_NEW, MainWindow::onMenuFileNew)
	EVT_MENU(wxID_OPEN, MainWindow::onMenuFileLoad)
	EVT_MENU(wxID_SAVE, MainWindow::onMenuFileSave)
	EVT_MENU(wxID_SAVEAS, MainWindow::onMenuFileSaveAs)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuFileExit)

	EVT_MENU(miEditUndo, MainWindow::onMenuEditUndo)
	EVT_MENU(miEditRedo, MainWindow::onMenuEditRedo)
	EVT_MENU(miEditReset, MainWindow::onMenuEditReset)
	EVT_MENU(miEditResetPlayers, MainWindow::onMenuEditResetPlayers)
	EVT_MENU(miEditResize, MainWindow::onMenuEditResize)
	EVT_MENU(miEditFlipX, MainWindow::onMenuEditFlipX)
	EVT_MENU(miEditFlipY, MainWindow::onMenuEditFlipY)
	EVT_MENU(miEditRandomizeHeights, MainWindow::onMenuEditRandomizeHeights)
	EVT_MENU(miEditRandomize, MainWindow::onMenuEditRandomize)
	EVT_MENU(miEditSwitchSurfaces, MainWindow::onMenuEditSwitchSurfaces)
	EVT_MENU(miEditInfo, MainWindow::onMenuEditInfo)
	EVT_MENU(miEditAdvanced, MainWindow::onMenuEditAdvanced)

	EVT_MENU(miMiscResetZoomAndPos, MainWindow::onMenuMiscResetZoomAndPos)
	EVT_MENU(miMiscAbout, MainWindow::onMenuMiscAbout)
	EVT_MENU(miMiscHelp, MainWindow::onMenuMiscHelp)
	EVT_MENU(miMiscShowMap, MainWindow::onMenuMiscShowMap)

	EVT_MENU_RANGE(miBrushHeight + 1, miBrushHeight + heightCount, MainWindow::onMenuBrushHeight)
	EVT_MENU_RANGE(miBrushGradient + 1, miBrushGradient + heightCount, MainWindow::onMenuBrushGradient)
	EVT_MENU_RANGE(miBrushSurface + 1, miBrushSurface + surfaceCount, MainWindow::onMenuBrushSurface)
	EVT_MENU_RANGE(miBrushObject + 1, miBrushObject + objectCount, MainWindow::onMenuBrushObject)
	EVT_MENU_RANGE(miBrushResource + 1, miBrushResource + resourceCount, MainWindow::onMenuBrushResource)
	EVT_MENU_RANGE(miBrushStartLocation + 1, miBrushStartLocation + startLocationCount, MainWindow::onMenuBrushStartLocation)
	EVT_MENU_RANGE(miRadius, miRadius + radiusCount, MainWindow::onMenuRadius)
	
	EVT_TOOL(toolPlayer, MainWindow::onToolPlayer)
END_EVENT_TABLE()

// =====================================================
// class GlCanvas
// =====================================================

GlCanvas::GlCanvas(MainWindow *mainWindow, wxWindow *parent, int *args)
		: wxGLCanvas(parent, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args)
		, mainWindow(mainWindow) {
}

void GlCanvas::onMouseDown(wxMouseEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	mainWindow->onMouseDown(event, x, y);
}

void GlCanvas::onMouseMove(wxMouseEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	mainWindow->onMouseMove(event, x, y);
}

void GlCanvas::onMousewheelRotation(wxMouseEvent &event) {
	if (event.GetWheelRotation() != 0)
		mainWindow->onMousewheelRotation(event);
}

void GlCanvas::onKeyDown(wxKeyEvent &event) {
	int x, y;
	event.GetPosition(&x, &y);
	mainWindow->onKeyDown(event);
}

void GlCanvas::onPaint(wxPaintEvent &event) {
	if(mainWindow->program){
		mainWindow->program->renderMap(this->GetClientSize().x, this->GetClientSize().y);
		this->SwapBuffers();
	}
	event.Skip();
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_KEY_DOWN(GlCanvas::onKeyDown)

	EVT_LEFT_DOWN(GlCanvas::onMouseDown)
	EVT_MOTION(GlCanvas::onMouseMove)
	EVT_MOUSEWHEEL(GlCanvas::onMousewheelRotation)
	
	EVT_PAINT(GlCanvas::onPaint)
END_EVENT_TABLE()

// ===============================================
//  class SimpleDialog
// ===============================================

void SimpleDialog::addValue(const string &key, const string &value) {
	values.push_back(pair<string, string>(key, value));
}

string SimpleDialog::getValue(const string &key) {
	for (size_t i = 0; i < values.size(); ++i) {
		if (values[i].first == key) {
			return values[i].second;
		}
	}
	return "";
}

void SimpleDialog::show() {

	Create(NULL, -1, wxT("Edit Values"));

	wxSizer *sizer = new wxFlexGridSizer(2);

	vector<wxTextCtrl*> texts;

	for (Values::iterator it = values.begin(); it != values.end(); ++it) {
		sizer->Add(new wxStaticText(this, -1, ToUnicode(it->first)), 0, wxALL, 5);
		wxTextCtrl *text = new wxTextCtrl(this, -1, ToUnicode(it->second));
		sizer->Add(text, 0, wxALL, 5);
		texts.push_back(text);
	}
	SetSizerAndFit(sizer);

	ShowModal();

	for (size_t i = 0; i < texts.size(); ++i) {
		values[i].second = texts[i]->GetValue().ToAscii();
	}
}

// ===============================================
//  class App
// ===============================================

bool App::OnInit() {
	FSFactory::getInstance()->usePhysFS = false;
	
	string fileparam, arg;
	wxString glest;
	for(int i=1; i<argc; ++i){
		arg = wxFNCONV(argv[i]);
		if(arg=="-glest" && (i+1)<argc){
			glest = argv[++i];
		}else{
			fileparam = arg;
		}
	}

	mainWindow = new MainWindow();
	mainWindow->Show();
	// needs to be after Show() otherwise assertGl() in Renderer::init will fail
	mainWindow->init(fileparam, glest);
	return true;
}

int App::MainLoop() {
	try {
		return wxApp::MainLoop();
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	return 0;
}

int App::OnExit() {
	return 0;
}

}// end namespace

IMPLEMENT_APP(MapEditor::App)
