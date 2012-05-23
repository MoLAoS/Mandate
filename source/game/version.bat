@echo off
setlocal enableextensions

REM set the git command depending on where git is installed
IF EXIST "C:\Git\bin\git.exe" (
	SET git_command="C:\Git\bin\git" describe
) ELSE (
	IF EXIST "C:\Git\bin\git.exe" (
		SET git_command="C:\Git\bin\git" describe
	) ELSE (
		ECHO Git is not installed in the expected location. Check source/game/version.bat
		exit /b 1
	)
)

REM batch's dodgy way of putting command output in a variable
FOR /F "tokens=*" %%a in ('%git_command%') do SET sha=%%a

REM write text to file
ECHO #include "pch.h">version.cpp
ECHO #include "version.h">>version.cpp
ECHO const char * build_date = "%date% %time%";>>version.cpp
ECHO const char * build_git_sha = "%sha%";>>version.cpp

endlocal