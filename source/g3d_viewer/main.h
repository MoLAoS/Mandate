#ifndef _SHADER_G3DVIEWER_MAIN_H_
#define _SHADER_G3DVIEWER_MAIN_H_

#include <string>

#include <wx/wx.h>
#include <wx/timer.h>
#include <wx/glcanvas.h>
#include <wx/colordlg.h>

#include "graphics_factory_basic_gl.h"
#include "renderer.h"
#include "util.h"
#include "window.h"

using std::string;

namespace Shared { namespace G3dViewer {

class GlCanvas;

class TeamColourDialog : public wxDialog {
	DECLARE_EVENT_TABLE()

	int editIndex;
	Renderer *renderer;

	wxButton *btnColour[4];
	wxColourDialog *colourDialog;

	wxStaticBitmap *bitmaps[4];

	void CreateChildren();

public:
	TeamColourDialog(Renderer *renderer);
	~TeamColourDialog();

	void onColourBtn(wxCommandEvent& event);

	wxColour colours[4];
};

// ===============================
// 	class MainWindow  
// ===============================

class MainWindow: public wxFrame {
private:
	DECLARE_EVENT_TABLE()

public:
	static const string versionString;
	static const string winHeader;

	enum MenuId{
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

	TeamColourDialog *colourDialog;

	Model *model;
	string modelPath;

	float speed;
	float anim;
	float rotX, rotY, zoom;
	int lastX, lastY;
	int playerColor;

public:
	MainWindow(const string &modelPath);
	~MainWindow();

	void init();

	void Notify();

	void onPaint(wxPaintEvent &event);
	void onClose(wxCloseEvent &event);
	void onMenuFileLoad(wxCommandEvent &event);
	void onMenuModeNormals(wxCommandEvent &event);
	void onMenuModeWireframe(wxCommandEvent &event);
	void onMenuModeGrid(wxCommandEvent &event);
	void onMenuSpeedSlower(wxCommandEvent &event);
	void onMenuSpeedFaster(wxCommandEvent &event);
	void onMenuColorOne(wxCommandEvent &event);
	void onMenuColorTwo(wxCommandEvent &event);
	void onMenuColorThree(wxCommandEvent &event);
	void onMenuColorFour(wxCommandEvent &event);
	void onMenuColorAll(wxCommandEvent &event);
	void onMenuColorEdit(wxCommandEvent &event);
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
