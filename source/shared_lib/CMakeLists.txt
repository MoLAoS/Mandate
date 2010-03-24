# glest shared lib

set(folders
	util
	math
	graphics
	graphics/gl
	platform
	sound
	tinyxml
	xml
	lua
	physfs
)

if(WIN32)
	list(APPEND folders platform/win32 sound/ds8)
else(WIN32)
	list(APPEND folders platform/sdl sound/openal)
endif(WIN32)

set(glestshared_incs "${CMAKE_CURRENT_SOURCE_DIR}/include")
foreach(folder ${folders})
	file(GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
		"sources/${folder}/*.cpp" "include/${folder}/*.h" "include/${folder}/*.hpp")
	list(APPEND glestshared_incs "${CMAKE_CURRENT_SOURCE_DIR}/include/${folder}")
	list(APPEND glestshared_srcs ${srcs})
	# to get subfolders in VC project
	if(WIN32)
		string(REPLACE "/" "\\" folder ${folder})
	endif(WIN32)
	source_group("${folder}" FILES ${srcs})
endforeach(folder)

# add glprocs.c needed on windows
if(WIN32)
	list(APPEND glestshared_srcs ${GLPROCS_FILE})
	source_group("" FILES ${GLPROCS_FILE})
endif(WIN32)

# ugly, but otherwise not the updated stuff
set(glestshared_incs ${glestshared_incs} PARENT_SCOPE)

include_directories(${glestshared_incs})
add_library(glestshared STATIC ${glestshared_srcs})
target_link_libraries(glestshared ${LUA_LIBRARIES} ${PHYSFS_LIBRARY})
if(WIN32)
	# FIXME: VC project needs static vorbis/ogg libs
	target_link_libraries(glestshared ${VORBISFILE_STATIC_LIBRARY} ${VORBIS_STATIC_LIBRARY} ${OGG_STATIC_LIBRARY})
else(WIN32)
	target_link_libraries(glestshared ${SDL_LIBRARY} ${OPENAL_LIBRARY} ${VORBISFILE_LIBRARY})
endif(WIN32)
