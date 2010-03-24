#include "main.h"

#include <stdexcept>

#include "graphics_interface.h"
#include "util.h"
#include "FSFactory.hpp"

using namespace Shared::Platform; 
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

using namespace std;

#if (wxUSE_UNICODE == 1)
#	define STRCONV(x) wxConvUTF8.cMB2WC(x)
#else
#	define STRCONV(x) x
#endif

namespace Shared { namespace G3dViewer {

TeamColourDialog::TeamColourDialog(Renderer *renderer) 
		: editIndex(0), renderer(renderer), colourDialog(0) {
	memset(btnColour, 0, sizeof(void*) * 4);
	memset(bitmaps, 0, sizeof(void*) * 4);

	for (int i=0; i < 4; ++i) {
		colours[i] = wxColour();
	}
	colours[0].Set(renderer->colours[3*0+0], renderer->colours[3*0+1], renderer->colours[3*0+2], 0xFF);
	colours[1].Set(renderer->colours[3*1+0], renderer->colours[3*1+1], renderer->colours[3*1+2], 0xFF);
	colours[2].Set(renderer->colours[3*2+0], renderer->colours[3*2+1], renderer->colours[3*2+2], 0xFF);
	colours[3].Set(renderer->colours[3*3+0], renderer->colours[3*3+1], renderer->colours[3*3+2], 0xFF);

	Create(0, 0, wxT("Customise Team Colours"));
	CreateChildren();
}

TeamColourDialog::~TeamColourDialog() {
	delete colourDialog;
}

void TeamColourDialog::CreateChildren() {
	SetSize(150, 150, 230, 340);

	for (int i=0; i < 4; ++i) {
		char buf[32];
		sprintf(buf, "Player %d Colour", (i+1));
		wxPoint pos(10, 80 * i + 10);
		wxSize size(160, 40);
		btnColour[i] = new wxButton(this, MainWindow::miCount + i, STRCONV(buf), pos, size);
		
		pos.x += 165;
		size.x = 40;
		size.y = 40;
		bitmaps[i] = new wxStaticBitmap(this, -1, wxNullBitmap, pos, size);
		bitmaps[i]->SetBackgroundColour(colours[i]);
	}
	colourDialog = new wxColourDialog();
}

void TeamColourDialog::onColourBtn(wxCommandEvent &event) {
	editIndex = event.GetId() - MainWindow::miCount;
	if (colourDialog->ShowModal() == wxID_OK) {
		wxColour colour = colourDialog->GetColourData().GetColour();
		colours[editIndex] = colour;
		bitmaps[editIndex]->SetBackgroundColour(colours[editIndex]);
		bitmaps[editIndex]->Refresh();
		renderer->resetTeamTexture(editIndex, colour.Red(), colour.Green(), colour.Blue());
	}
}

BEGIN_EVENT_TABLE(TeamColourDialog, wxDialog)
	EVT_BUTTON(MainWindow::miCount + 0, TeamColourDialog::onColourBtn)
	EVT_BUTTON(MainWindow::miCount + 1, TeamColourDialog::onColourBtn)
	EVT_BUTTON(MainWindow::miCount + 2, TeamColourDialog::onColourBtn)
	EVT_BUTTON(MainWindow::miCount + 3, TeamColourDialog::onColourBtn)
END_EVENT_TABLE()

// ===============================================
// 	class MainWindow
// ===============================================

const string MainWindow::versionString= "v1.3.4+";
const string MainWindow::winHeader= "G3D viewer " + versionString + " - Built: " + __DATE__;

MainWindow::MainWindow(const string &modelPath)
		: wxFrame( NULL, -1, STRCONV(winHeader.c_str()),
			wxPoint(Renderer::windowX, Renderer::windowY), 
			wxSize(Renderer::windowW, Renderer::windowH) ) {
	renderer= Renderer::getInstance();
	this->modelPath = modelPath;
	model = NULL;
	playerColor = 0;

	speed= 0.025f;
	
	//gl canvas
	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER };
	glCanvas = new GlCanvas(this, args);
	
	menu= new wxMenuBar();

	//menu
	menuFile= new wxMenu();
	menuFile->Append(miFileLoad, wxT("Load"));
	menu->Append(menuFile, wxT("File"));

	//mode
	menuMode= new wxMenu();
	menuMode->AppendCheckItem(miModeNormals, wxT("Normals"));
	menuMode->AppendCheckItem(miModeWireframe, wxT("Wireframe"));
	menuMode->AppendCheckItem(miModeGrid, wxT("Grid"));
	menu->Append(menuMode, wxT("Mode"));

	//mode
	menuSpeed= new wxMenu();
	menuSpeed->Append(miSpeedSlower, wxT("Slower"));
	menuSpeed->Append(miSpeedFaster, wxT("Faster"));
	menu->Append(menuSpeed, wxT("Speed"));

	//custom color
	menuCustomColor= new wxMenu();
	menuCustomColor->AppendCheckItem(miColorOne, wxT("Faction 1"));
	menuCustomColor->AppendCheckItem(miColorTwo, wxT("Faction 2"));
	menuCustomColor->AppendCheckItem(miColorThree, wxT("Faction 3"));
	menuCustomColor->AppendCheckItem(miColorFour, wxT("Faction 4"));
	menuCustomColor->AppendCheckItem(miColourAll, wxT("Show All"));
	menuCustomColor->Append(miColourEdit, wxT("Edit Colours"));
	menu->Append(menuCustomColor, wxT("Custom Color"));

	menuMode->Check(miModeGrid, true);
	menuCustomColor->Check(miColorOne, true);

	SetMenuBar(menu);

	//misc
	model= NULL;
	rotX= 0.0f;
	rotY= 0.0f;
	zoom= 1.0f;
	lastX= 0;
	lastY= 0;
	anim= 0.0f;

	CreateStatusBar();

	colourDialog = new TeamColourDialog(renderer);

	timer = new wxTimer(this);
	timer->Start(40);
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
		GetStatusBar()->SetStatusText(STRCONV(getModelInfo().c_str()));
	}
}

void MainWindow::onPaint(wxPaintEvent &event) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (playerColor == -1) {
		int w = GetClientSize().x / 2;
		int h = GetClientSize().y / 2;
		int x = 0, y = 0;
		for (++playerColor; playerColor < 4; ++playerColor) {
			renderer->reset(x, y, w, h, playerColor);
			renderer->transform(rotX, rotY, zoom);
			renderer->renderGrid();
			
			renderer->renderTheModel(model, anim);
			if (!x) {
				x = w;
			} else {
				y = h;
				x = 0;
			}
		}
		playerColor = -1;
	} else {
		renderer->reset(0, 0, GetClientSize().x, GetClientSize().y, playerColor);
		renderer->transform(rotX, rotY, zoom);
		renderer->renderGrid();
		
		renderer->renderTheModel(model, anim);
	}
	glCanvas->SwapBuffers();
}

void MainWindow::onClose(wxCloseEvent &event){
	delete this;
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
		delete model;
		Model *tmpModel= new ModelGl();
		fileName = wxFNCONV(fileDialog.GetPath());
		renderer->loadTheModel(tmpModel, fileName);
		model= tmpModel;
		GetStatusBar()->SetStatusText(wxString(getModelInfo().c_str(), wxConvUTF8));
	}
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
	speed/= 1.5f;
}

void MainWindow::onMenuSpeedFaster(wxCommandEvent &event){
	speed*= 1.5f;
}

void MainWindow::onMenuColorOne(wxCommandEvent &event){
	playerColor = 0;
	menuCustomColor->Check(miColorOne, true);
	menuCustomColor->Check(miColorTwo, false);
	menuCustomColor->Check(miColorThree, false);
	menuCustomColor->Check(miColorFour, false);
	menuCustomColor->Check(miColourAll, false);
}

void MainWindow::onMenuColorTwo(wxCommandEvent &event){
	playerColor = 1;
	menuCustomColor->Check(miColorOne, false);
	menuCustomColor->Check(miColorTwo, true);
	menuCustomColor->Check(miColorThree, false);
	menuCustomColor->Check(miColorFour, false);
	menuCustomColor->Check(miColourAll, false);
}

void MainWindow::onMenuColorThree(wxCommandEvent &event){
	playerColor = 2;
	menuCustomColor->Check(miColorOne, false);
	menuCustomColor->Check(miColorTwo, false);
	menuCustomColor->Check(miColorThree, true);
	menuCustomColor->Check(miColorFour, false);
	menuCustomColor->Check(miColourAll, false);
}

void MainWindow::onMenuColorFour(wxCommandEvent &event){
	playerColor = 3;
	menuCustomColor->Check(miColorOne, false);
	menuCustomColor->Check(miColorTwo, false);
	menuCustomColor->Check(miColorThree, false);
	menuCustomColor->Check(miColorFour, true);
	menuCustomColor->Check(miColourAll, false);
}

void MainWindow::onMenuColorAll(wxCommandEvent &event){
	playerColor = -1;
	menuCustomColor->Check(miColorOne, false);
	menuCustomColor->Check(miColorTwo, false);
	menuCustomColor->Check(miColorThree, false);
	menuCustomColor->Check(miColorFour, false);
	menuCustomColor->Check(miColourAll, true);
}

void MainWindow::onMenuColorEdit(wxCommandEvent &event){	
	colourDialog->Show();
}

void MainWindow::onTimer(wxTimerEvent &event){
	wxPaintEvent paintEvent;	
	anim = anim + speed;
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

	return str;
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_TIMER(-1, MainWindow::onTimer)
	EVT_CLOSE(MainWindow::onClose)
	EVT_MENU(miFileLoad, MainWindow::onMenuFileLoad)

	EVT_MENU(miModeWireframe, MainWindow::onMenuModeWireframe)
	EVT_MENU(miModeNormals, MainWindow::onMenuModeNormals)
	EVT_MENU(miModeGrid, MainWindow::onMenuModeGrid)

	EVT_MENU(miSpeedFaster, MainWindow::onMenuSpeedFaster)
	EVT_MENU(miSpeedSlower, MainWindow::onMenuSpeedSlower)

	EVT_MENU(miColorOne, MainWindow::onMenuColorOne)
	EVT_MENU(miColorTwo, MainWindow::onMenuColorTwo)
	EVT_MENU(miColorThree, MainWindow::onMenuColorThree)
	EVT_MENU(miColorFour, MainWindow::onMenuColorFour)
	EVT_MENU(miColourAll, MainWindow::onMenuColorAll)
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
	FSFactory::getInstance()->usePhysFS(false);
	
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
		wxMessageDialog(NULL, STRCONV(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	return 0;
}

int App::OnExit(){
	return 0;
}

}}//end namespace

IMPLEMENT_APP(Shared::G3dViewer::App)
