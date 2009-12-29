# glest shared lib

set(folders
	util
	graphics
	graphics/gl
	platform
	sound
	tinyxml
	xml
	lua
)

if(WIN32)
	list(APPEND folders platform/win32 sound/ds8)
else(WIN32)
	list(APPEND folders platform/posix platform/sdl sound/openal)
endif(WIN32)

set(shared_lib_srcs "")
set(shared_lib_incs "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "include directories for shared lib")
foreach(folder ${folders})
	file(GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "sources/${folder}/*.cpp" "include/${folder}/*.h")
	list(APPEND shared_lib_incs "${CMAKE_CURRENT_SOURCE_DIR}/include/${folder}")
	list(APPEND shared_lib_srcs ${srcs})
	# to get subfolders in VC project
	if(WIN32)
		string(REPLACE "/" "\\" folder ${folder})
	endif(WIN32)
	source_group("${folder}" FILES ${srcs})
endforeach(folder)

if(WIN32)
	list(APPEND shared_lib_srcs ${GLPROCS_FILE})
	source_group("" FILES ${GLPROCS_FILE})
endif(WIN32)

# ugly, but otherwise not in cache
set(shared_lib_incs ${shared_lib_incs} CACHE INTERNAL "include directories for shared lib")

include_directories(${shared_lib_incs})
add_library(shared_lib STATIC ${shared_lib_srcs})
if(WIN32)
	target_link_libraries(shared_lib ${DSOUND_LIBRARY} wsock32)
else(WIN32)
	target_link_libraries(shared_lib ${SDL_LIBRARY} ${OPENAL_LIBRARY})
endif(WIN32)
