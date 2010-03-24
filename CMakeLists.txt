cmake_minimum_required (VERSION 2.6)

# build type defaults to debug
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif(NOT DEFINED CMAKE_BUILD_TYPE)
if(WIN32 AND MSVC)
	# change default compile flags for VC on windows
	if(NOT DEFINED CMAKE_CXX_FLAGS)
		set(CMAKE_CXX_FLAGS "/DWIN32 /D_WINDOWS /W2 /Zm1000 /EHsc /GR- /Ob2 /arch:SSE2 /fp:fast /Zc:forScope-" CACHE STRING "Flags used by the compiler during all build types.")
	endif()
	if(NOT DEFINED CMAKE_CXX_FLAGS_RELEASE)
		set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /DNDEBUG /Oi /Ot" CACHE STRING "Flags used by the compiler during release builds (/MD /Ob1 /Oi /Ot /Oy /Gs will produce slightly less optimized but smaller files).")
	endif()
endif(WIN32 AND MSVC)

project(glestae CXX C)

# add additional CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

#AC_CHECK_HEADERS([glob.h sys/ioctl.h sys/filio.h sys/time.h byteswap.h])
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
CHECK_INCLUDE_FILE(glob.h HAVE_GLOB_H)
CHECK_INCLUDE_FILE(sys/ioctl.h HAVE_SYS_IOCTL_H)
CHECK_INCLUDE_FILE(sys/filio.h HAVE_SYS_FILIO_H)
CHECK_INCLUDE_FILE(sys/time.h HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE(byteswap.h HAVE_BYTESWAP_H)

# set paths for config and data
if(NOT DEFINED CONFIG_DIR)
	set(CONFIG_DIR "" CACHE PATH "user specific directory for configs and logs")
endif(NOT DEFINED CONFIG_DIR)
if(NOT DEFINED DATA_DIR)
	set(DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/glestae/" CACHE PATH "root directory of data")
endif(NOT DEFINED DATA_DIR)

# help the windows people with their deps folder
if(DEFINED WINDEPS)
	message(STATUS "setting windeps...")
	#set(ENV{SDLDIR} ${WINDEPS})
	set(ENV{LUA_DIR} ${WINDEPS})
	set(ENV{VORBISDIR} ${WINDEPS})
	set(ENV{PHYSFSDIR} ${WINDEPS})
	# sadly FindZLIB does not support HINTS, set it directly
	set(ZLIB_INCLUDE_DIR ${WINDEPS}/include)
	set(ZLIB_LIBRARY ${WINDEPS}/lib/zlib.lib)
	# FindLibXml2 uses PkgConfig as HINTS on non win, set them directly
	set(PC_LIBXML_INCLUDEDIR ${WINDEPS}/include)
	set(PC_LIBXML_LIBDIR ${WINDEPS}/lib)
	# root folder of wxWidgets
	if(DEFINED WXWIN)
		set(ENV{WXWIN} ${WXWIN})
	endif(DEFINED WXWIN)
endif(DEFINED WINDEPS)

find_package(Lua51 5.1 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PhysFS REQUIRED)
find_package(CppUnit)
find_package(LibXml2)
find_package(wxWidgets COMPONENTS base core gl)

include_directories(${LUA_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${PHYSFS_INCLUDE_DIR})

if(WIN32)
	# dsound8 library
	find_path(DSOUND_INCLUDE_DIR dsound.h HINTS ${WINDEPS} PATH_SUFFIXES include)
	find_library(DSOUND_LIBRARY NAMES dsound HINTS ${WINDEPS} PATH_SUFFIXES lib)
	find_library(DXGUID_LIBRARY NAMES dxguid HINTS ${WINDEPS} PATH_SUFFIXES lib)
	# needs to be compiled as C (done automatically because *.c and project() has C), extra file for shared_lib
	find_file(GLPROCS_FILE NAMES glprocs.c HINTS ${WINDEPS}/src)
	include_directories(${DSOUND_INCLUDE_DIR})
	# FIXME: VC project needs to be build with static vorbis/ogg libs
	find_library(VORBISFILE_STATIC_LIBRARY NAMES vorbisfile_static HINTS ${WINDEPS} PATH_SUFFIXES lib)
	find_library(VORBIS_STATIC_LIBRARY NAMES vorbis_static HINTS ${WINDEPS} PATH_SUFFIXES lib)
	find_library(OGG_STATIC_LIBRARY NAMES ogg_static HINTS ${WINDEPS} PATH_SUFFIXES lib)
else(WIN32)
	find_package(SDL 1.2 REQUIRED)
	find_package(OpenAL REQUIRED)
	include_directories(${SDL_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR})
	# a bit ugly
	set(USE_SDL "ON")
	set(USE_POSIX_SOCKETS "ON")
	set(X11_AVAILABLE "ON")
endif(WIN32)

if(NOT DEFINED GLEST_VERSION)
	set(GLEST_VERSION "svn" CACHE STRING "Version of glest advanced engine")
endif()
if(NOT DEFINED CONTACT)
	set(CONTACT "glestae-devel@lists.sourceforge.net" CACHE STRING "Contact Mail Address")
endif()

# cpack stuff
# FIXME: setting version from outside does not work fully
set(CPACK_PACKAGE_NAME "glestae")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "glest advanced engine")
set(CPACK_PACKAGE_VENDOR "gae developers")
set(CPACK_PACKAGE_VERSION ${GLEST_VERSION})
# set(CPACK_PACKAGE_VERSION_MAJOR "1")
# set(CPACK_PACKAGE_VERSION_MINOR "2")
# set(CPACK_PACKAGE_VERSION_PATCH "3")
set(CPACK_PACKAGE_CONTACT ${CONTACT})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/docs/license.txt)
if(wxWidgets_FOUND)
	set(CPACK_PACKAGE_EXECUTABLES
			"glestadv" "Glest Advanced Engine"
			"glest_editor" "Map Editor"
			"glest_configurator" "Configurator"
			"glest_g3dviewer" "G3D Viewer"
	)
else(wxWidgets_FOUND)
	set(CPACK_PACKAGE_EXECUTABLES "glestadv" "Glest Advanced Engine")
endif(wxWidgets_FOUND)

if(WIN32)
	# install runtime libs from VC
	include(InstallRequiredSystemLibraries)

	set(CPACK_GENERATOR "NSIS")
	set(CPACK_NSIS_DISPLAY_NAME "glest advanced engine")
	set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.glest.org")
	set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.glest.org")
	set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
	set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
	set(CPACK_NSIS_MODIFY_PATH "ON")
	set(CPACK_NSIS_MENU_LINKS
		"share/glestae" "data folder"
	)
	# a bug in NSIS, last one needs to be \ and 2 times escaped to get there
	set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/data/game\\\\glest.ico")
	
	if(DEFINED WINDEPS)
		# only lua and physfs are linked dynamic, install it next to the binary
		install(FILES ${WINDEPS}/bin/lua5.1.dll DESTINATION bin)
		install(FILES ${WINDEPS}/bin/physfs.dll DESTINATION bin)
	endif(DEFINED WINDEPS)
else(WIN32)
	# for linux, default: tarball, to get deb-package run 'cpack -G DEB'
	# TODO: try to add some general dependencies for deb and rpm
	set(CPACK_RPM_PACKAGE_REQUIRES "zlib >= 1.2, physfs >= 2.0, libogg >= 1.1, libvorbis >= 1.2, lua >= 5.1, SDL >= 1.2")

	set(CPACK_GENERATOR "TBZ2")
	set(CPACK_STRIP_FILES "TRUE")
endif(WIN32)

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/.git/;/.svn/;/data/;/build/;.*~;/docs/html/;/mk/")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "glestae-src")
# get it written to CPackConfig.cmake
include(CPack)


# create projectConfig.h, used in source files
configure_file(${PROJECT_SOURCE_DIR}/projectConfig.h.in ${PROJECT_BINARY_DIR}/projectConfig.h)
include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(source)


# data directory
# if(NOT WIN32)
# 	install(FILES data/glest.xpm DESTINATION share/pixmaps)
# 	install(FILES data/glest.desktop DESTINATION share/applications)
# endif(NOT WIN32)
install(DIRECTORY data/game/data data/game/maps data/game/techs data/game/tilesets data/game/gae
 		DESTINATION share/glestae REGEX "/.svn" EXCLUDE)

if(NOT WIN32)
	# add uninstall target; FIXME: does not delete created folders
	configure_file(
		"${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY)
	add_custom_target(uninstall
		"${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake")
endif(NOT WIN32)
