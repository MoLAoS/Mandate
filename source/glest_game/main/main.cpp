// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "main.h"

#include <string>
#include <cstdlib>

#include "game.h"
#include "main_menu.h"
#include "program.h"
#include "config.h"
#include "metrics.h"
#include "game_util.h"
#include "platform_util.h"
#include "platform_main.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler{
public:
	virtual void handle(string description, void *address){
		FILE *f= fopen("crash.txt", "at");
		if(f!=NULL){
			time_t t= time(NULL);
			char *timeString= asctime(localtime(&t));

			fprintf(f, "Crash\n");
			fprintf(f, "Version: Advanced Engine %s\n", gaeVersionString.c_str());
			fprintf(f, "Time: %s", timeString);
			fprintf(f, "Description: %s\n", description.c_str());
			fprintf(f, "Address: %p\n\n", address);
			fclose(f);
		}

		message("An error ocurred and Glest will close.\nCrash info has been saved in the crash.txt file\nPlease report this bug to " + gaeMailString);
	}
};

// =====================================================
// 	class MainWindow
// =====================================================

MainWindow::MainWindow(Program *program){
	this->program= program;
}

MainWindow::~MainWindow(){
	delete program;
}

void MainWindow::eventMouseDown(int x, int y, MouseButton mouseButton){
	switch(mouseButton){
	case mbLeft:
		program->mouseDownLeft(x, getH() - y);
		break;
	case mbRight:
		program->mouseDownRight(x, getH() - y);
		break;
	case mbCenter:
		program->mouseDownCenter(x, getH() - y);
		break;
	default:
		break;
	}
}

void MainWindow::eventMouseUp(int x, int y, MouseButton mouseButton){
	switch(mouseButton){
	case mbLeft:
		program->mouseUpLeft(x, getH() - y);
		break;
	case mbCenter:
		program->mouseUpCenter(x, getH() - y);
		break;
	default:
		break;
	}
}

void MainWindow::eventMouseDoubleClick(int x, int y, MouseButton mouseButton){
	if(mouseButton == mbLeft){
		program->mouseDoubleClickLeft(x,  getH() - y);
	}
}

void MainWindow::eventMouseWheel(int x, int y, int zDelta){
	program->eventMouseWheel(x,  getH() - y, zDelta);
}

void MainWindow::eventMouseMove(int x, int y, const MouseState *ms){
	program->mouseMove(x, getH() - y, ms);
}

void MainWindow::eventKeyDown(char key){
	program->keyDown(key);
}

void MainWindow::eventKeyUp(char key){
	program->keyUp(key);
}

void MainWindow::eventKeyPress(char c){
	program->keyPress(c);
}

void MainWindow::eventActivate(bool active){
	if(!active){
		//minimize();
	}
}

void MainWindow::eventResize(SizeState sizeState){
	program->resize(sizeState);
}

void MainWindow::eventClose(){
	delete program;
	program= NULL;
}

// =====================================================
// Main
// =====================================================

int glestMain(int argc, char** argv){

	MainWindow *mainWindow= NULL;
	Program *program= NULL;
	ExceptionHandler exceptionHandler;
	exceptionHandler.install();

	try{
		Config &config = Config::getInstance();

		showCursor(config.getWindowed());

		program= new Program();
		mainWindow= new MainWindow(program);

		//parse command line
		if(argc==2 && string(argv[1])=="-server"){
			program->initServer(mainWindow);
		}
		else if(argc==3 && string(argv[1])=="-client"){
			program->initClient(mainWindow, Ip(argv[2]));
		}
		else{
			program->initNormal(mainWindow);
		}

		//main loop
		while(Window::handleEvent()){
			program->loop();
		}
	}
	catch(const exception &e){
		restoreVideoMode();
		exceptionMessage(e);
	}

	delete mainWindow;

	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMain)
