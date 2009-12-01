// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2005 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under 
//	the terms of the GNU General Public License as published 
//	by the Free Software Foundation; either version 2 of the 
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_MENUSTATEOPTIONS_H_
#define _GLEST_GAME_MENUSTATEOPTIONS_H_

#include "main_menu.h"

namespace Glest{ namespace Game{

// ===============================
// 	class MenuStateOptions  
// ===============================

class MenuStateOptions: public MenuState{
private:
	GraphicButton buttonReturn;	
	GraphicButton buttonAutoConfig;	
	GraphicButton buttonOpenglInfo;
	
	GraphicLabel labelLang;
	GraphicLabel labelShadows;
	GraphicLabel labelFilter;
	GraphicLabel labelTextures3D;
	GraphicLabel labelLights;
	GraphicLabel labelVolumeFx;
	GraphicLabel labelVolumeAmbient;
	GraphicLabel labelVolumeMusic;
	GraphicListBox listBoxLang;
	GraphicListBox listBoxShadows;
	GraphicListBox listBoxFilter;
	GraphicListBox listBoxTextures3D;
	GraphicListBox listBoxLights;
	GraphicListBox listBoxVolumeFx;
	GraphicListBox listBoxVolumeAmbient;
	GraphicListBox listBoxVolumeMusic;
	GraphicListBox listBoxMusicSelect;

	GraphicLabel labelMaxPathNodes;
	GraphicListBox listBoxMaxPathNodes;
	GraphicLabel labelPFAlgorithm;
	GraphicListBox listBoxPFAlgorithm;
#	ifdef _GAE_DEBUG_EDITION_
		GraphicLabel labelPFTexturesOn;
		GraphicLabel labelPFTextureMode;
		GraphicListBox listBoxPFTexturesOn;
		GraphicListBox listBoxPFTextureMode;
#	endif

	map<string,string> langMap;

private:
	MenuStateOptions(const MenuStateOptions &);
	const MenuStateOptions &operator =(const MenuStateOptions &);

public:
	MenuStateOptions(Program &program, MainMenu *mainMenu);

	void mouseClick(int x, int y, MouseButton mouseButton);
	void mouseMove(int x, int y, const MouseState &mouseState);
	void render();

private:
	void saveConfig();
	void setupListBoxLang();
	void initLabels();
	void initListBoxes();
	void setTexts();
};

}}//end namespace

#endif
