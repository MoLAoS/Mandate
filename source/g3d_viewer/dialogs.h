#ifndef _G3DVIEWER_DIALOG_H_
#define _G3DVIEWER_DIALOG_H_

#include <string>

#include <wx/wx.h>
#include <wx/colordlg.h>

#include "renderer.h"

using std::string;

namespace Shared { namespace G3dViewer {

// ===============================
// 	class TeamColourDialog
// ===============================

class TeamColourDialog : public wxDialog {
	DECLARE_EVENT_TABLE()

	int editIndex;
	Renderer *renderer;

	wxButton *btnColour[8];
	wxColourDialog *colourDialog;

	wxStaticBitmap *bitmaps[8];

	void CreateChildren();

	static const int idStart = 0xF00;

public:
	TeamColourDialog(Renderer *renderer);
	~TeamColourDialog();

	void onColourBtn(wxCommandEvent& event);

	wxColour colours[8];
};

// ===============================
// 	class TeamColourDialog
// ===============================

class ModelInfo : public wxDialog {
	DECLARE_EVENT_TABLE()

	Renderer *renderer;
	void CreateChildren();

public:
	ModelInfo(Model *model);
	~ModelInfo();
};

}}

#endif
