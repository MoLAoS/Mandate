cmake_minimum_required (VERSION 2.6)

# build type defaults to debug
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif(NOT DEFINED CMAKE_BUILD_TYPE)

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
if(NOT DEFINED GAE_CONFIG_DIR)
	set(GAE_CONFIG_DIR "" CACHE PATH "user specific directory for configs and logs")
endif(NOT DEFINED GAE_CONFIG_DIR)
if(NOT DEFINED GAE_DATA_DIR)
	set(GAE_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/glestae/" CACHE PATH "root directory of data")
endif(NOT DEFINED GAE_DATA_DIR)

# set number of processors to use (currently only Visual Studio)
if(NOT DEFINED GAE_COMPILE_PROCESSORS)
	set(GAE_COMPILE_PROCESSORS "1" CACHE STRING "The number of processors to use for compiling. Leave blank to use the number of effective processors on your computer.")
endif(NOT DEFINED GAE_COMPILE_PROCESSORS)

# help the windows people with their deps folder
if(DEFINED GAE_WINDEPS AND WIN32)
	message(STATUS "setting windeps: " ${GAE_WINDEPS})
	#set(ENV{SDLDIR} ${GAE_WINDEPS})
	set(ENV{LUA_DIR} ${GAE_WINDEPS})
	set(ENV{VORBISDIR} ${GAE_WINDEPS})
	set(ENV{PHYSFSDIR} ${GAE_WINDEPS})
	set(ENV{CPPUNITDIR} ${GAE_WINDEPS})
	# sadly FindZLIB does not support HINTS, set it directly (seemingly the same with PNG...)
	set(PNG_LIBRARY ${GAE_WINDEPS}/lib/libpng.lib)
	set(PNG_PNG_INCLUDE_DIR ${GAE_WINDEPS}/include)
	set(JPEG_LIBRARY ${GAE_WINDEPS}/lib/jpeg-static.lib)
	set(JPEG_INCLUDE_DIR ${GAE_WINDEPS}/include/jpeg)
	set(ZLIB_INCLUDE_DIR ${GAE_WINDEPS}/include)
	set(ZLIB_LIBRARY ${GAE_WINDEPS}/lib/zlib.lib)
	# FindLibXml2 uses PkgConfig as HINTS on non win, set them directly
	set(PC_LIBXML_INCLUDEDIR ${GAE_WINDEPS}/include)
	set(PC_LIBXML_LIBDIR ${GAE_WINDEPS}/lib)
	# root folder of wxWidgets
	if(DEFINED WXWIN)
		#message("WXWIN Defined")
		set(ENV{WXWIN} ${WXWIN})
	else(DEFINED WXWIN)
		#message("WXWIN Not Defined, setting wxWidgets_ROOT_DIR to windeps/wx")
		set(wxWidgets_ROOT_DIR ${GAE_WINDEPS}/wx)
	endif(DEFINED WXWIN)
	set(ENV{FREETYPE_DIR} ${GAE_WINDEPS})
	set(ENV{GLEW_DIR} ${GAE_WINDEPS}/glew-1.5.7)
else(DEFINED GAE_WINDEPS AND WIN32)
	if (WIN32)
		set (GAE_WINDEPS "" CACHE PATH "Windows dependencies folder")
		message("Warning: GAE_WINDEPS is not set, delete cache and set GAE_WINDEPS.")
	endif(WIN32)
endif(DEFINED GAE_WINDEPS AND WIN32)

find_package(Lua51 5.1 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CppUnit)
find_package(LibXml2)
find_package(wxWidgets COMPONENTS base core gl)
find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(GLEW REQUIRED)
find_package(PhysFS REQUIRED)

# Option: debug edition
option(GAE_DEBUG_EDITION "Enable debug edition extensions.")

# Option: leak dumping
option(GAE_LEAK_DUMP "Enable leak dumping, for detecting memory leaks.")

# Option: pre-compiled headers (MSVC only atm)
if (MSVC)
	option(GAE_USE_PRECOMPILED_HDR "Enable pre-compiled headers." ON)
endif (MSVC)

# Option: include the XAudio2 SoundPlayer
if (WIN32)
	option(GAE_USE_XAUDIO2 "Include XAudio2 SoundPlayer" OFF)
endif(WIN32)

if(NOT WIN32)
	try_compile(HAVE_SSE2 ${CMAKE_BINARY_DIR}/test_sse ${CMAKE_SOURCE_DIR}/cmake/tests/sse.cpp COMPILE_DEFINITIONS "-msse2")
	if(NOT HAVE_SSE2)
		message(FATAL_ERROR "SSE2 support is needed, sorry")
	endif(NOT HAVE_SSE2)

	try_compile(HAVE_PHYSFS_2 ${CMAKE_BINARY_DIR}/test_physfs ${CMAKE_SOURCE_DIR}/cmake/tests/physfs.cpp
				CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${PHYSFS_INCLUDE_DIR}" "-DLINK_LIBRARIES=${PHYSFS_LIBRARY}")
	if(NOT HAVE_PHYSFS_2)
		message(FATAL_ERROR "physfs 2.0 or later is required")
	endif(NOT HAVE_PHYSFS_2)
endif(NOT WIN32)

# set compiler flags...
if (MSVC)
	set(CMAKE_C_FLAGS "/wd4250 /wd4355 /wd4996 /wd4018")
	set(CMAKE_CXX_FLAGS "/DWIN32 /W2 /wd4250 /wd4355 /wd4996 /GR- /Ob2 /GF /EHsc /fp:fast /arch:SSE2 /MP${GAE_COMPILE_PROCESSORS}")
	set(CMAKE_CXX_FLAGS_DEBUG  "/MTd /Od /Gy /RTC1 /Zi /DDEBUG")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /O2 -g /Zi /DNDEBUG") 
	set(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ot /Oy /Oi /GL /DNDEBUG /Zi")
else (MSVC)
	# Use defaults ? 
	#set(CMAKE_CXX_FLAGS "-Wall -fno-rtti")
	if(NOT CMAKE_CXX_FLAGS MATCHES -msse2)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2" CACHE STRING "" FORCE)
	endif()
endif (MSVC)

include_directories(${LUA_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR}
		${PHYSFS_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${PNG_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

if(WIN32)
	# dsound8 library
	find_path(DSOUND_INCLUDE_DIR dsound.h HINTS ${GAE_WINDEPS} PATH_SUFFIXES include)
	find_library(DSOUND_LIBRARY NAMES dsound HINTS ${GAE_WINDEPS} PATH_SUFFIXES lib)
	find_library(DXGUID_LIBRARY NAMES dxguid HINTS ${GAE_WINDEPS} PATH_SUFFIXES lib)
	# FIXME: VC project needs to be build with static vorbis/ogg libs
	find_library(VORBISFILE_STATIC_LIBRARY NAMES vorbisfile_static HINTS ${GAE_WINDEPS} PATH_SUFFIXES lib)
	find_library(VORBIS_STATIC_LIBRARY NAMES vorbis_static HINTS ${GAE_WINDEPS} PATH_SUFFIXES lib)
	find_library(OGG_STATIC_LIBRARY NAMES ogg_static HINTS ${GAE_WINDEPS} PATH_SUFFIXES lib)
else(WIN32)
	find_package(SDL 1.2 REQUIRED)
	find_package(OpenAL REQUIRED)
	include_directories(${SDL_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR})
	# a bit ugly
	set(USE_SDL "ON")
	set(USE_POSIX_SOCKETS "ON")
	set(X11_AVAILABLE "ON")
endif(WIN32)

if(NOT DEFINED GAE_VERSION)
	set(GAE_VERSION "git-master" CACHE STRING "Version of Glest Advanced Engine")
endif()
if(NOT DEFINED GAE_CONTACT)
	set(GAE_CONTACT "glestae-devel@lists.sourceforge.net" CACHE STRING "Contact Mail Address")
endif()

# cpack stuff
# FIXME: setting version from outside does not work fully
set(CPACK_PACKAGE_NAME "glestae")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Glest Advanced Engine")
set(CPACK_PACKAGE_VENDOR "The GAE Team")
set(CPACK_PACKAGE_VERSION ${GAE_VERSION})
# set(CPACK_PACKAGE_VERSION_MAJOR "1")
# set(CPACK_PACKAGE_VERSION_MINOR "2")
# set(CPACK_PACKAGE_VERSION_PATCH "3")
set(CPACK_PACKAGE_CONTACT ${GAE_CONTACT})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/docs/license.txt)
if(wxWidgets_FOUND)
	set(CPACK_PACKAGE_EXECUTABLES
			"glestadv" "Glest Advanced Engine"
			"gae_mapeditor" "Map Editor"
			#"configurator" "Configurator"
			"gae_g3dviewer" "G3D Viewer"
	)
else(wxWidgets_FOUND)
	set(CPACK_PACKAGE_EXECUTABLES "glestadv" "Glest Advanced Engine")
endif(wxWidgets_FOUND)

if(WIN32)
	set(CPACK_GENERATOR "NSIS")
	set(CPACK_NSIS_DISPLAY_NAME "Glest Advanced Engine")
	set(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.glest.org")
	set(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.glest.org")
	set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
	set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
	set(CPACK_NSIS_MODIFY_PATH "ON")

	set(CPACK_CREATE_DESKTOP_LINKS "glestadv")
	# doesn't work because target is prefixed with $INSTDIR; now in extra_install_commands
	#set(CPACK_NSIS_MENU_LINKS "$PROFILE/glestadv/addons" "Addons Folder")
	# a bug in NSIS, last one needs to be \ and 2 times escaped to get there
	set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/source/game\\\\glestadv_inst.ico")
	set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/source/game\\\\glestadv_uninst.ico")
	
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
		CreateDirectory \\\"$PROFILE\\\\glestadv\\\\addons\\\"
		CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Addons Folder.lnk\\\" \\\"$PROFILE\\\\glestadv\\\\addons\\\"
	")
	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
		Delete \\\"$SMPROGRAMS\\\\$MUI_TEMP\\\\Addons Folder.lnk\\\"
	")

	message( "Bin dir is: ${CMAKE_BINARY_DIR}")

	if(DEFINED GAE_WINDEPS)
		# freetype, zlib & libpng are linked dynamically, install them next to the binary
		install(FILES 
			${GAE_WINDEPS}/bin/freetype6.dll
			${GAE_WINDEPS}/bin/zlib1.dll
			${GAE_WINDEPS}/bin/libpng12-0.dll
			${GAE_WINDEPS}/bin/glew32.dll
			DESTINATION bin
		)
		# dbghelp.dll to get nice crash reports, install program db for debugging symbols
		install(FILES
			${CMAKE_BINARY_DIR}/source/game/Release/glestadv.pdb
			${CMAKE_BINARY_DIR}/source/game/Release/glestadv.map
			${GAE_WINDEPS}/bin/dbghelp.dll
			DESTINATION bin
		)
	endif(DEFINED GAE_WINDEPS)

# FIXME: not tested
# elseif(APPLE)
# 	set(CPACK_GENERATOR "Bundle")
# 	set(CPACK_PACKAGE_FILE_NAME "GlestAE")
# 	set(CPACK_BUNDLE_NAME "GlestAE")
# 	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/glestadv.icns")
# 	set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/cmake/glestadv.icns")
# 	set(CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/macstartup.sh")
# 	configure_file(${CMAKE_SOURCE_DIR}/cmake/info.plist.in ${CMAKE_BINARY_DIR}/info.plist)
# 	set(CPACK_BUNDLE_PLIST "${CMAKE_BINARY_DIR}/info.plist")
# 
# 	# install dependencies
# 	install(FILES
# 		${LUA_LIBRARIES} ${ZLIB_LIBRARIES} ${FREETYPE_LIBRARIES} ${PNG_LIBRARY}
# 		${SDL_LIBRARY} ${OPENAL_LIBRARIES} ${VORBISFILE_LIBRARY}
# 		${OPENGL_gl_LIBRARIES} ${OPENGL_glu_LIBRARIEIES} ${wxWidgets_LIBRARIES}
# 		DESTINATION lib
# 	)

else()
	# for linux, default: tarball, to get deb-package run 'cpack -G DEB'
	# TODO: try to add some general dependencies for deb and rpm
	
	# RPM
	#set(CPACK_RPM_PACKAGE_REQUIRES "zlib >= 1.2, libogg >= 1.1, libvorbis >= 1.2, lua >= 5.1, SDL >= 1.2")
	
	# Debian package
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpng12-0, libvorbis0a, libogg0, lua5.1, libsdl1.2debian, libphysfs1")
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "The GAE Team <glestae-devel@lists.sourceforge.net>")
	set(CPACK_DEBIAN_PACKAGE_SECTION "games")

	set(CPACK_GENERATOR "TBZ2")
endif()
set(CPACK_STRIP_FILES "TRUE")

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/.git/;.gitignore;/.svn/;.kdev;.supp;.patch;.db;/build;.*~;/docs/html/;/package")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "glestae-src-${GAE_VERSION}")
# get it written to CPackConfig.cmake
include(CPack)

# create projectConfig.h, used in source files
configure_file(${CMAKE_SOURCE_DIR}/projectConfig.h.in ${CMAKE_BINARY_DIR}/projectConfig.h)
include_directories(${CMAKE_BINARY_DIR})

# ctest
#include(CTest)
ENABLE_TESTING()
#set(MEMORYCHECK_COMMAND /usr/bin/valgrind)
# add_test() called in subdirectory


# doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${CMAKE_BINARY_DIR}/Doxyfile)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile)
else(DOXYGEN_FOUND)
	message(STATUS "disabled doc")
endif(DOXYGEN_FOUND)


option(GAE_PKG_ONLY_DATA "" OFF)
option(GAE_PKG_ONLY_BIN "" OFF)

# add stuff
if(NOT GAE_PKG_ONLY_DATA)
	add_subdirectory(source)
endif()

# data directory
if(NOT GAE_PKG_ONLY_BIN)
	install(DIRECTORY data/game/data data/game/maps data/game/techs data/game/tilesets data/game/gae
			DESTINATION share/glestae OPTIONAL REGEX "/.svn" EXCLUDE)
endif()

if(NOT WIN32)
	# add uninstall target; FIXME: does not delete created folders
	configure_file(
		"${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY)
	add_custom_target(uninstall
		"${CMAKE_COMMAND}" -P "${CMAKE_BINARY_DIR}/cmake_uninstall.cmake")
endif(NOT WIN32)
