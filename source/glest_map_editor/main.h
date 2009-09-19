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

#ifndef _MAPEDITOR_MAIN_H_
#define _MAPEDITOR_MAIN_H_

#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "program.h"

using std::string;
using std::vector;
using std::pair;

namespace MapEditor {

class GlCanvas;

// =====================================================
//	class MainWindow
// =====================================================

class MainWindow: public wxFrame {
private:
	DECLARE_EVENT_TABLE()

private:
	static const string versionString;
	static const string winHeader;
	static const int heightCount = 11;
	static const int surfaceCount = 5;
	static const int objectCount = 11;
	static const int resourceCount = 6;
	static const int startLocationCount = 4;
	static const int radiusCount = 9;

private:
	enum MenuId {
		miFileLoad,
		miFileSave,
		miFileSaveAs,
		miFileExit,

		miEditReset,
		miEditResetPlayers,
		miEditResize,
		miEditFlipX,
		miEditFlipY,
		miEditRandomizeHeights,
		miEditRandomize,
		miEditSwitchSurfaces,
		miEditInfo,
		miEditAdvanced,

		miMiscResetZoomAndPos,
		miMiscAbout,
		miMiscHelp,

		miBrushHeight,
		miBrushSurface = miBrushHeight + heightCount + 1,
		miBrushObject = miBrushSurface + surfaceCount + 1,
		miBrushResource = miBrushObject + objectCount + 1,
		miBrushStartLocation = miBrushResource + resourceCount + 1,

		miRadius = miBrushStartLocation + startLocationCount + 1
	};

private:
	GlCanvas *glCanvas;
	Program *program;
	int lastX, lastY;

	wxMenuBar *menuBar;
	wxMenu *menuFile;
	wxMenu *menuEdit;
	wxMenu *menuMisc;
	wxMenu *menuBrush;
	wxMenu *menuBrushHeight;
	wxMenu *menuBrushSurface;
	wxMenu *menuBrushObject;
	wxMenu *menuBrushResource;
	wxMenu *menuBrushStartLocation;
	wxMenu *menuRadius;

	string currentFile;

	int height;
	int surface;
	int radius;
	int object;
	int resource;
	int startLocation;
	int enabledGroup;

public:
	MainWindow();
	~MainWindow();

	void init();

	void onClose(wxCloseEvent &event);
	void onMouseDown(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);
	void onPaint(wxPaintEvent &event);

	void onMenuFileLoad(wxCommandEvent &event);
	void onMenuFileSave(wxCommandEvent &event);
	void onMenuFileSaveAs(wxCommandEvent &event);
	void onMenuFileExit(wxCommandEvent &event);

	void onMenuEditReset(wxCommandEvent &event);
	void onMenuEditResetPlayers(wxCommandEvent &event);
	void onMenuEditResize(wxCommandEvent &event);
	void onMenuEditFlipX(wxCommandEvent &event);
	void onMenuEditFlipY(wxCommandEvent &event);
	void onMenuEditRandomizeHeights(wxCommandEvent &event);
	void onMenuEditRandomize(wxCommandEvent &event);
	void onMenuEditSwitchSurfaces(wxCommandEvent &event);
	void onMenuEditInfo(wxCommandEvent &event);
	void onMenuEditAdvanced(wxCommandEvent &event);

	void onMenuMiscResetZoomAndPos(wxCommandEvent &event);
	void onMenuMiscAbout(wxCommandEvent &event);
	void onMenuMiscHelp(wxCommandEvent &event);

	void onMenuBrushHeight(wxCommandEvent &event);
	void onMenuBrushSurface(wxCommandEvent &event);
	void onMenuBrushObject(wxCommandEvent &event);
	void onMenuBrushResource(wxCommandEvent &event);
	void onMenuBrushStartLocation(wxCommandEvent &event);
	void onMenuRadius(wxCommandEvent &event);

	void change(int x, int y);

	void uncheckBrush();
	void uncheckRadius();
};

// =====================================================
//	class GlCanvas
// =====================================================

class GlCanvas: public wxGLCanvas {
private:
	DECLARE_EVENT_TABLE()

public:
	GlCanvas(MainWindow *mainWindow);

	void onMouseDown(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);

private:
	MainWindow *mainWindow;
};

// =====================================================
//	class SimpleDialog
// =====================================================

class SimpleDialog: public wxDialog {
private:
	typedef vector<pair<string, string> > Values;

private:
	Values values;

public:
	void addValue(const string &key, const string &value);
	string getValue(const string &key);

	void show();
};

// =====================================================
//	class App
// =====================================================

// =====================================================
//	class App
// =====================================================

class App: public wxApp {
private:
	MainWindow *mainWindow;

public:
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}// end namespace

DECLARE_APP(MapEditor::App)

#endif
