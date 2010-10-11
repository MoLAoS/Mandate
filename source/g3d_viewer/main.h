#ifndef _G3DVIEWER_MAIN_H_
#define _G3DVIEWER_MAIN_H_

#include <string>

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/glcanvas.h>
#include <wx/colordlg.h>

#include "renderer.h"
#include "util.h"
#include "window.h"
#include "dialogs.h"

using std::string;

#if (wxUSE_UNICODE == 1)
#	define STRCONV(x) wxConvUTF8.cMB2WC(x)
#else
#	define STRCONV(x) x
#endif

namespace Shared { namespace G3dViewer {

class GlCanvas;

WRAPPED_ENUM( StatusItems,
	NULL_ENTRY,
	MODEL_INFO,
	ANIM_SPEED
)

// ===============================
// 	class MainWindow
// ===============================

class MainWindow: public wxFrame {
private:
	DECLARE_EVENT_TABLE()

public:
	static const string versionString;
	static const string winHeader;

	enum MenuId {
		miFileLoad,
		miModeWireframe,
		miModeNormals,
		miModeGrid,
		miSpeedSlower,
		miSpeedFaster,
		miColorOne,
		miColorTwo,
		miColorThree,
		miColorFour,
		miColorFive,
		miColorSix,
		miColorSeven,
		miColorEight,
		miColourAll,
		miColourEdit,

		miCount
	};

private:
	GlCanvas *glCanvas;
	Renderer *renderer;

	wxTimer *timer;

	wxMenuBar *menu;
	wxMenu *menuFile;
	wxMenu *menuMode;
	wxMenu *menuSpeed;
	wxMenu *menuCustomColor;
	wxMenu *menuMesh;

	TeamColourDialog *colourDialog;

	Model *model;

	string modelPath;

	int speed;
	float anim;
	float rotX, rotY, zoom;
	int lastX, lastY;
	int playerColor;

	void buildStatusBar();

public:
	MainWindow(const string &modelPath);
	~MainWindow();

	void init();

	void Notify();

	void onPaint(wxPaintEvent &event);
	void onClose(wxCloseEvent &event);
	void onMenuFileLoad(wxCommandEvent &event);
	void onMenuExit(wxCommandEvent &event);
	void onMenuModeNormals(wxCommandEvent &event);
	void onMenuModeWireframe(wxCommandEvent &event);
	void onMenuModeGrid(wxCommandEvent &event);
	void onMenuSpeedSlower(wxCommandEvent &event);
	void onMenuSpeedFaster(wxCommandEvent &event);
	void onMenuColor(wxCommandEvent &event);
	void onMenuColorEdit(wxCommandEvent &event);
	void onMenuMeshSelect(wxCommandEvent &evt);
	void onMouseMove(wxMouseEvent &event);
	void onTimer(wxTimerEvent &event);

	string getModelInfo();
};

// =====================================================
//	class GlCanvas
// =====================================================

class GlCanvas: public wxGLCanvas{
private:
	DECLARE_EVENT_TABLE()

public:
	GlCanvas(MainWindow *mainWindow, int *args);

	void onMouseMove(wxMouseEvent &event);
	void onPaint(wxPaintEvent &event);

private:
	MainWindow *mainWindow;
};

// ===============================
// 	class App  
// ===============================

class App: public wxApp{
private:
	MainWindow *mainWindow;

public:
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}}//end namespace

DECLARE_APP(Shared::G3dViewer::App)

#endif
