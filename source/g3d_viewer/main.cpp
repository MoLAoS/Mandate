#include "pch.h"

#include "main.h"

#include <stdexcept>

#include "graphics_interface.h"
#include "model_gl.h"
#include "conversion.h"
#include "util.h"
#include "FSFactory.hpp"

//#include "particle_type.h"

#include "gae_g3dviewer.xpm"

using std::exception;
using namespace Shared::Platform; 
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Shared { namespace G3dViewer {

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
// 	class MainWindow
// ===============================================

const string MainWindow::versionString= "v1.3.4+";
const string MainWindow::winHeader= "G3D viewer " + versionString + " - Built: " + __DATE__;

MainWindow::MainWindow(const string &modelPath)
		: wxFrame( NULL, -1, ToUnicode(winHeader),
			wxPoint(Renderer::windowX, Renderer::windowY), 
			wxSize(Renderer::windowW, Renderer::windowH) ) {
	renderer= Renderer::getInstance();
	this->modelPath = modelPath;
	model = NULL;
	playerColor = 0;

	speed = 100;
	
	//gl canvas
	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER };
	glCanvas = new GlCanvas(this, args);
	
	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("Load Model\tCTRL+O"));
	menuFile->Append(wxID_EXIT);
	menu->Append(menuFile, wxT("&File"));

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, wxT("Normals"));
	menuMode->AppendCheckItem(miModeWireframe, wxT("Wireframe"));
	menuMode->AppendCheckItem(miModeGrid, wxT("Grid"));
	menu->Append(menuMode, wxT("&Mode"));

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, wxT("Slower"));
	menuSpeed->Append(miSpeedFaster, wxT("Faster"));
	menu->Append(menuSpeed, wxT("&Speed"));

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->AppendCheckItem(miColorOne, wxT("Faction 1"));
	menuCustomColor->AppendCheckItem(miColorTwo, wxT("Faction 2"));
	menuCustomColor->AppendCheckItem(miColorThree, wxT("Faction 3"));
	menuCustomColor->AppendCheckItem(miColorFour, wxT("Faction 4"));
	menuCustomColor->AppendCheckItem(miColorFive, wxT("Faction 5"));
	menuCustomColor->AppendCheckItem(miColorSix, wxT("Faction 6"));
	menuCustomColor->AppendCheckItem(miColorSeven, wxT("Faction 7"));
	menuCustomColor->AppendCheckItem(miColorEight, wxT("Faction 8"));
	menuCustomColor->AppendCheckItem(miColourAll, wxT("Show All"));
	menuCustomColor->Append(miColourEdit, wxT("Edit Colours"));
	menu->Append(menuCustomColor, wxT("&Custom Color"));

	// mesh
	menuMesh = new wxMenu();
	menuMesh->AppendCheckItem(miCount, wxT("Show All"));
	menu->Append(menuMesh, wxT("Mesh"));
	Connect(miCount, wxEVT_COMMAND_MENU_SELECTED,
		wxCommandEventHandler(MainWindow::onMenuMeshSelect), NULL, this);

	menuMesh->Check(miCount, true);
	menuMode->Check(miModeGrid, true);
	menuCustomColor->Check(miColorOne, true);

	SetMenuBar(menu);

	SetIcon(wxIcon(gae_g3dviewer_xpm));

	//misc
	model= NULL;
	rotX= 0.0f;
	rotY= 0.0f;
	zoom= 1.0f;
	lastX= 0;
	lastY= 0;
	anim= 0.0f;

	buildStatusBar();

	colourDialog = new TeamColourDialog(renderer);

	timer = new wxTimer(this);
	timer->Start(25);
}

void MainWindow::buildStatusBar() {
	int status_widths[StatusItems::COUNT] = {
		10, // empty
		-3, // model info
		-1  // anim speed
	};
	CreateStatusBar(StatusItems::COUNT);
	GetStatusBar()->SetStatusWidths(StatusItems::COUNT, status_widths);

	SetStatusText(wxT(""), StatusItems::MODEL_INFO);
	SetStatusText(wxT("Anim speed: 100"), StatusItems::ANIM_SPEED);
}

MainWindow::~MainWindow(){
	delete renderer;
	delete model;
	delete timer;
	delete glCanvas;
	delete colourDialog;
}

void MainWindow::init(){
	glCanvas->SetCurrent();
	renderer->init();
	if(!modelPath.empty()){
		Model *tmpModel= new ModelGl();
		renderer->loadTheModel(tmpModel, modelPath);
		model= tmpModel;
		GetStatusBar()->SetStatusText(ToUnicode(getModelInfo()), StatusItems::MODEL_INFO);
	}
}

void MainWindow::onPaint(wxPaintEvent &event) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (playerColor == -1) {
		int w = GetClientSize().x / 4;
		int h = GetClientSize().y / 2;
		int x = 0, y = 0;
		for (int i = 0; i < 8; ++i) {
			renderer->reset(x, y, w, h, i);
			renderer->transform(rotX, rotY, zoom);
			renderer->renderGrid();
			
			renderer->renderTheModel(model, anim);
			if (x != w * 3) {
				x += w;
			} else {
				y = h;
				x = 0;
			}
		}
	} else {
		renderer->reset(0, 0, GetClientSize().x, GetClientSize().y, playerColor);
		renderer->transform(rotX, rotY, zoom);
		renderer->renderGrid();
		
		renderer->renderTheModel(model, anim);
	}
	glCanvas->SwapBuffers();
}

void MainWindow::onClose(wxCloseEvent &event){
	//delete this;
	Destroy();
}

void MainWindow::onMouseMove(wxMouseEvent &event){
	
	int x= event.GetX();
	int y= event.GetY();
	wxPaintEvent paintEvent;

	if(event.LeftIsDown()){
		rotX+= clamp(lastX-x, -10, 10);
		rotY+= clamp(lastY-y, -10, 10);
		onPaint(paintEvent);
	} 
	else if(event.RightIsDown()){
		zoom*= 1.0f+(lastX-x+lastY-y)/100.0f;
		zoom= clamp(zoom, 0.1f, 10.0f);
		onPaint(paintEvent);
	}

	lastX= x;
	lastY= y;
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event){
	string fileName;
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("G3D files (*.g3d)|*.g3d"));
	if(fileDialog.ShowModal()==wxID_OK){
		if (model) {
			// remove mesh menu items
			for (int i=0; i < model->getMeshCount(); ++i) {
				wxMenuItem *item = menuMesh->Remove(miCount + i + 1);
				delete item;
			}
		}
		delete model;
		Model *tmpModel= new ModelGl();
		fileName = wxFNCONV(fileDialog.GetPath());
		renderer->loadTheModel(tmpModel, fileName);
		model= tmpModel;
		GetStatusBar()->SetStatusText(ToUnicode(getModelInfo()), StatusItems::MODEL_INFO);
		for (int i=0; i < model->getMeshCount(); ++i) {
			wxMenuItem *item = menuMesh->AppendCheckItem(miCount + i + 1, ToUnicode(intToStr(i+1)));
			Connect(miCount + i + 1, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(MainWindow::onMenuMeshSelect), NULL, this);
		}
		menuMesh->Check(miCount, true);
		renderer->setMesh();
	}
}

void MainWindow::onMenuExit(wxCommandEvent& event){
	Close();
}

void MainWindow::onMenuModeNormals(wxCommandEvent &event){
	renderer->toggleNormals();
	menuMode->Check(miModeNormals, renderer->getNormals());
}

void MainWindow::onMenuModeWireframe(wxCommandEvent &event){
	renderer->toggleWireframe();
	menuMode->Check(miModeWireframe, renderer->getWireframe());
}

void MainWindow::onMenuModeGrid(wxCommandEvent &event){
	renderer->toggleGrid();
	menuMode->Check(miModeGrid, renderer->getGrid());
}

void MainWindow::onMenuSpeedSlower(wxCommandEvent &event){
	if (speed > 0) {
		speed -= 25;
		GetStatusBar()->SetStatusText(ToUnicode("Anim speed: " + intToStr(speed)), StatusItems::ANIM_SPEED);
	}
}

void MainWindow::onMenuSpeedFaster(wxCommandEvent &event){
	speed += 25;
	GetStatusBar()->SetStatusText(ToUnicode("Anim speed: " + intToStr(speed)), StatusItems::ANIM_SPEED);
}

void MainWindow::onMenuColor(wxCommandEvent &evt){
	playerColor = evt.GetId() - miColorOne;
	if (playerColor >= 8) {
		playerColor = -1;
	}
	for (int i = miColorOne; i <= miColourAll; ++i) {
		menuCustomColor->Check(i, false);
	}
	menuCustomColor->Check(evt.GetId(), true);
}

void MainWindow::onMenuColorEdit(wxCommandEvent &event){	
	colourDialog->Show();
}

void MainWindow::onMenuMeshSelect(wxCommandEvent &evt) {
	if (evt.GetId() == miCount) {
		renderer->setMesh();
	} else {
		int n = evt.GetId() - miCount - 1;
		renderer->setMesh(n);
	}
	if (model) {
		for (int i=0; i <= model->getMeshCount(); ++i) {
			menuMesh->Check(miCount + i, false);
		}
		menuMesh->Check(evt.GetId(), true);
	}
}

void MainWindow::onTimer(wxTimerEvent &event){
	wxPaintEvent paintEvent;
	float inc = 0.00025f * speed;
	anim += inc;
	if (anim > 1.0f) {
		anim -= 1.f;
	}
	onPaint(paintEvent);
}	

string MainWindow::getModelInfo(){
	string str;

	if(model!=NULL){
		str+= "Meshes: "+intToStr(model->getMeshCount());
		str+= ", Vertices: "+intToStr(model->getVertexCount());
		str+= ", Triangles: "+intToStr(model->getTriangleCount());
		str+= ", Version: "+intToStr(model->getFileVersion());
	}
	int maxFrames = 0;
	for (int i=0; i < model->getMeshCount(); ++i) {
		const Mesh *mesh = model->getMesh(i);
		if (mesh->getFrameCount() > maxFrames) {
			maxFrames = mesh->getFrameCount();
		}
	}
	str += ", ";
	str += intToStr(maxFrames) + " Frames.";

	return str;
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuExit)

	EVT_MENU(miModeWireframe, MainWindow::onMenuModeWireframe)
	EVT_MENU(miModeNormals, MainWindow::onMenuModeNormals)
	EVT_MENU(miModeGrid, MainWindow::onMenuModeGrid)

	EVT_MENU(miSpeedFaster, MainWindow::onMenuSpeedFaster)
	EVT_MENU(miSpeedSlower, MainWindow::onMenuSpeedSlower)

	EVT_MENU(miColorOne, MainWindow::onMenuColor)
	EVT_MENU(miColorTwo, MainWindow::onMenuColor)
	EVT_MENU(miColorThree, MainWindow::onMenuColor)
	EVT_MENU(miColorFour, MainWindow::onMenuColor)
	EVT_MENU(miColorFive, MainWindow::onMenuColor)
	EVT_MENU(miColorSix, MainWindow::onMenuColor)
	EVT_MENU(miColorSeven, MainWindow::onMenuColor)
	EVT_MENU(miColorEight, MainWindow::onMenuColor)
	EVT_MENU(miColourAll, MainWindow::onMenuColor)
	EVT_MENU(miColourEdit, MainWindow::onMenuColorEdit)
END_EVENT_TABLE()

// =====================================================
//	class GlCanvas
// =====================================================

GlCanvas::GlCanvas(MainWindow *	mainWindow, int *args)
		: wxGLCanvas(mainWindow, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->mainWindow = mainWindow;
}

void GlCanvas::onMouseMove(wxMouseEvent &event){
	mainWindow->onMouseMove(event);
}

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_MOTION(GlCanvas::onMouseMove)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit(){
	FSFactory::getInstance()->usePhysFS = false;
	
	string modelPath;
	if(argc==2){
		modelPath= wxFNCONV(argv[1]);
	}
	
	mainWindow= new MainWindow(modelPath);
	mainWindow->Show();
	mainWindow->init();
	return true;
}

int App::MainLoop(){
	try{
		return wxApp::MainLoop();
	}
	catch(const exception &e){
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	return 0;
}

int App::OnExit(){
	return 0;
}

}}//end namespace

IMPLEMENT_APP(Shared::G3dViewer::App)
