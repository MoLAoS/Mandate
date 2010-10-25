#include "main.h"

namespace Shared { namespace G3dViewer {

// ===============================
// 	class TeamColourDialog
// ===============================

TeamColourDialog::TeamColourDialog(Renderer *renderer) 
		: editIndex(0), renderer(renderer), colourDialog(0) {
	memset(btnColour, 0, sizeof(void*) * 4);
	memset(bitmaps, 0, sizeof(void*) * 4);

	for (int i=0; i < 8; ++i) {
		colours[i] = wxColour();
		Vec3<uint8> colour = renderer->getTeamColour(i);
		colours[i].Set(colour.r, colour.g, colour.b, 0xFFu);
	}

	Create(0, 0, wxT("Customise Team Colours"), wxDefaultPosition, wxDefaultSize, 
		wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP);
	CreateChildren();
}

TeamColourDialog::~TeamColourDialog() {
	delete colourDialog;
}

void TeamColourDialog::CreateChildren() {

	SetSize(150, 150, 230, 450);

	for (int i=0; i < 8; ++i) {
		char buf[32];
		sprintf(buf, "Player %d Colour", (i+1));
		wxPoint pos(10, 50 * i + 10);
		wxSize size(160, 40);
		btnColour[i] = new wxButton(this, idStart + i, ToUnicode(buf), pos, size);
		
		pos.x += 165;
		size.x = 40;
		size.y = 40;
		bitmaps[i] = new wxStaticBitmap(this, -1, wxNullBitmap, pos, size);
		bitmaps[i]->SetBackgroundColour(colours[i]);
	}
	colourDialog = new wxColourDialog();
}

void TeamColourDialog::onColourBtn(wxCommandEvent &event) {
	editIndex = event.GetId() - idStart;
	if (colourDialog->ShowModal() == wxID_OK) {
		wxColour colour = colourDialog->GetColourData().GetColour();
		colours[editIndex] = colour;
		bitmaps[editIndex]->SetBackgroundColour(colours[editIndex]);
		bitmaps[editIndex]->Refresh();
		renderer->resetTeamTexture(editIndex, colour.Red(), colour.Green(), colour.Blue());
	}
}

BEGIN_EVENT_TABLE(TeamColourDialog, wxDialog)
	EVT_BUTTON(idStart + 0, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 1, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 2, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 3, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 4, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 5, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 6, TeamColourDialog::onColourBtn)
	EVT_BUTTON(idStart + 7, TeamColourDialog::onColourBtn)
END_EVENT_TABLE()

}}
