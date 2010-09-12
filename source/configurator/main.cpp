#include "main.h"

#include <stdexcept>

#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/icon.h>

using namespace std;

namespace Configurator {

// ===============================================
// 	class MainWindow
// ===============================================

const int MainWindow::margin = 10;
const int MainWindow::panelMargin = 20;
const int MainWindow::gridMarginHorizontal = 30;

MainWindow::MainWindow() {
	SetExtraStyle(wxFRAME_EX_CONTEXTHELP);

	configuration.load("configuration.xml");

	Create(NULL, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE | wxMINIMIZE_BOX | wxMAXIMIZE_BOX);

	SetTitle(STRCONV(("Configurator - " + configuration.getTitle() + " - Editing " + configuration.getFileName()).c_str()));

	if (configuration.getIcon()) {
		wxIcon icon;
		icon.LoadFile(STRCONV(configuration.getIconPath().c_str()), wxBITMAP_TYPE_ICO);
		SetIcon(icon);
	}

	notebook = new wxNotebook(this, -1);

	wxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(notebook, 1, wxGROW | wxALL, margin);

	for (int fgI = 0; fgI < configuration.getFieldGroupCount(); ++fgI) {
		//create page
		FieldGroup *fg = configuration.getFieldGroup(fgI);
		wxPanel *panel = new wxPanel(notebook, -1);
		notebook->AddPage(panel, STRCONV(fg->getName().c_str()));

		//sizers
		wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2, margin, gridMarginHorizontal);
		wxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);
		panelSizer->Add(gridSizer, 0, wxGROW | wxALL | wxALIGN_CENTER, panelMargin);
		panel->SetSizer(panelSizer);
		panelSizer->Fit(panel);

		for (int fI = 0; fI < fg->getFieldCount(); ++fI) {
			Field *f = fg->getField(fI);
			FieldText *staticText = new FieldText(panel, this, f);
			staticText->SetAutoLayout(true);
			gridSizer->Add(staticText);
			gridSizer->AddGrowableCol((fI * 2) + 1);
			f->createControl(panel, gridSizer);
			idMap.insert(IdPair(staticText->GetId(), staticText));
		}
	}

	//buttons
	wxSizer *bottomSizer = new wxBoxSizer(wxHORIZONTAL);

	buttonOk = new wxButton(this, biOk, wxT("OK"));
	buttonApply = new wxButton(this, biApply, wxT("Apply"));
	buttonCancel = new wxButton(this, biCancel, wxT("Cancel"));
	buttonDefault = new wxButton(this, biDefault, wxT("Default"));
	bottomSizer->Add(buttonOk, 0, wxALL, margin);
	bottomSizer->Add(buttonApply, 0, wxRIGHT | wxBOTTOM | wxTOP, margin);
	bottomSizer->Add(buttonCancel, 0, wxRIGHT | wxBOTTOM | wxTOP, margin);
	bottomSizer->Add(buttonDefault, 0, wxRIGHT | wxBOTTOM | wxTOP, margin);

	infoText = new wxTextCtrl(this, -1, wxT("Info text."), wxDefaultPosition,
							wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY);
	infoText->SetBackgroundColour(buttonOk->GetBackgroundColour());

	mainSizer->Add(infoText, 0, wxGROW | wxALL, margin);
	mainSizer->Add(bottomSizer, 0, wxALIGN_CENTER);

	SetBackgroundColour(buttonOk->GetBackgroundColour());

	SetSizerAndFit(mainSizer);

	Refresh();
}

void MainWindow::onButtonOk(wxCommandEvent &event) {
	configuration.save();
	Close();
}

void MainWindow::onButtonApply(wxCommandEvent &event) {
	configuration.save();
}

void MainWindow::onButtonCancel(wxCommandEvent &event) {
	Close();
}

void MainWindow::onButtonDefault(wxCommandEvent &event) {
	for (int fgI = 0; fgI < configuration.getFieldGroupCount(); ++fgI) {
		FieldGroup *fg = configuration.getFieldGroup(fgI);

		for (int fI = 0; fI < fg->getFieldCount(); ++fI) {
			Field *f = fg->getField(fI);
			f->setValue(f->getDefaultValue());
			f->updateControl();
		}
	}
}

void MainWindow::onClose(wxCloseEvent &event) {
	Destroy();
}

void MainWindow::onMouseDown(wxMouseEvent &event) {
	setInfoText("Info text.");
}

void MainWindow::setInfoText(const string &str) {
	infoText->SetValue(STRCONV(str.c_str()));
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)
	EVT_BUTTON(biOk, MainWindow::onButtonOk)
	EVT_BUTTON(biApply, MainWindow::onButtonApply)
	EVT_BUTTON(biCancel, MainWindow::onButtonCancel)
	EVT_BUTTON(biDefault, MainWindow::onButtonDefault)
	EVT_CLOSE(MainWindow::onClose)
	EVT_LEFT_DOWN(MainWindow::onMouseDown)
END_EVENT_TABLE()

// ===============================================
// 	class FieldText
// ===============================================

FieldText::FieldText(wxWindow *parent, MainWindow *mainWindow, const Field *field):
	wxStaticText(parent, -1, STRCONV(field->getName().c_str()))
 {
	this->mainWindow = mainWindow;
	this->field = field;
}

void FieldText::onHelp(wxHelpEvent &event) {
	string str = field->getInfo() + ".";

	if (!field->getDescription().empty()) {
		str += "\n" + field->getDescription() + ".";
	}

	mainWindow->setInfoText(str);
}


BEGIN_EVENT_TABLE(FieldText, wxStaticText)
	EVT_HELP(-1, FieldText::onHelp)
END_EVENT_TABLE()

// ===============================================
// 	class App
// ===============================================

bool App::OnInit() {
	try {
		mainWindow = new MainWindow();
		mainWindow->Show();
	}
	catch (const exception &e) {
		wxMessageDialog(NULL, STRCONV(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
		return 0;
	}

	return true;
}

int App::MainLoop() {
	try {
		return wxApp::MainLoop();
	}
	catch (const exception &e) {
		wxMessageDialog(NULL, STRCONV(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();

		return 0;
	}
}

int App::OnExit() {
	return 0;
}

}//end namespace

IMPLEMENT_APP(Configurator::App)
