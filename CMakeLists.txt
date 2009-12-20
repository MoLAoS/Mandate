cmake_minimum_required (VERSION 2.6)

# build type defaults to debug
if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif(NOT DEFINED CMAKE_BUILD_TYPE)

project(glestae)

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
	set(CONFIG_DIR "" CACHE FILEPATH "user specific directory for configs and logs")
endif(NOT DEFINED CONFIG_DIR)
if(NOT DEFINED DATA_DIR)
	set(DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/glestae/" CACHE FILEPATH "root directory of data")
endif(NOT DEFINED DATA_DIR)
#message(STATUS "c:${CONFIG_DIR};;d:${DATA_DIR};")

# help the windows people with their deps folder
if(DEFINED WINDEPS)
	message(STATUS "setting windeps...")
	#set(ENV{SDLDIR} ${WINDEPS})
	set(ENV{LUA_DIR} ${WINDEPS})
	set(ENV{VORBISDIR} ${WINDEPS})
	set(ZLIB_INCLUDE_DIR ${WINDEPS}/include)
	set(ZLIB_LIBRARY ${WINDEPS}/lib/zlib.lib)
endif(DEFINED WINDEPS)

find_package(Lua51 5.1 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(ZLIB REQUIRED)
find_package(wxWidgets COMPONENTS base core gl)

include_directories(${LUA_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${VORBIS_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

if(WIN32)
	# TODO: look for dsound8 and winapi stuff
	# dsound8 library
	find_path(DSOUND_INCLUDE_DIR dsound.h HINTS ${WINDEPS} PATH_SUFFIXES include)
	find_library(DSOUND_LIBRARY NAMES dsound HINTS ${WINDEPS} PATH_SUFFIXES lib)
	include_directories(${DSOUND_INCLUDE_DIR})
else(WIN32)
	find_package(SDL 1.2 REQUIRED)
	find_package(OpenAL REQUIRED)
	include_directories(${SDL_INCLUDE_DIR} ${OPENAL_INCLUDE_DIR})
	# FIXME: better included from projectConfig.h, needs changes in sources
	#add_definitions(-DUSE_SDL -DUSE_POSIX_SOCKETS -DHAVE_SYS_IOCTL_H -DHAVE_SYS_TIME_H -DX11_AVAILABLE)
	# FIXME: a bit ugly
	set(USE_SDL "ON")
	set(USE_POSIX_SOCKETS "ON")
	set(X11_AVAILABLE "ON")
endif(WIN32)

# create projectConfig.h, TODO: use it in source files
configure_file(${PROJECT_SOURCE_DIR}/projectConfig.h.in ${PROJECT_BINARY_DIR}/projectConfig.h)
include_directories(${PROJECT_BINARY_DIR})

add_subdirectory(source)


# cpack stuff
set(CPACK_GENERATOR "TBZ2")
set(CPACK_PACKAGE_NAME "glestae")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "glest advanced engine")
set(CPACK_PACKAGE_VENDOR "gae developers")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "2")
set(CPACK_PACKAGE_VERSION_PATCH "3")
set(CPACK_PACKAGE_CONTACT "blubbmonster@mail.domain")
set(CPACK_STRIP_FILES "TRUE")

set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/.git/;/.svn/;/data/;/build/;.*~;/docs/html/;/mk/")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "glestae-src")
# get it written to CPackConfig.cmake
include(CPack)

# data directory
#install(FILES data/glest.xpm DESTINATION share/pixmaps)
#install(FILES data/glest.desktop DESTINATION share/applications)
install(DIRECTORY data/game/data data/game/maps data/game/techs data/game/tilesets data/game/gae
 		DESTINATION share/glestae REGEX "/.svn" EXCLUDE)

# add uninstall target; FIXME: seems not working?
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)
add_custom_target(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
