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
	physfs
)

if(WIN32)
	list(APPEND folders platform/win32 sound/ds8)
else(WIN32)
	list(APPEND folders platform/posix platform/sdl sound/openal)
endif(WIN32)

set(glestshared_srcs "")
set(glestshared_incs "${CMAKE_CURRENT_SOURCE_DIR}/include" CACHE INTERNAL "include directories for shared lib")
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

if(WIN32)
	list(APPEND glestshared_srcs ${GLPROCS_FILE})
	source_group("" FILES ${GLPROCS_FILE})
endif(WIN32)

# ugly, but otherwise not in cache
set(glestshared_incs ${glestshared_incs} CACHE INTERNAL "include directories for shared lib")

include_directories(${glestshared_incs})
add_library(glestshared STATIC ${glestshared_srcs})
#add_library(glestshared SHARED ${glestshared_srcs})
target_link_libraries(glestshared ${LUA_LIBRARIES})
if(WIN32)
	#target_link_libraries(glestshared ${DSOUND_LIBRARY} wsock32)
	# FIXME: VC project needs static vorbis/ogg libs
	target_link_libraries(glestshared ${VORBISFILE_STATIC_LIBRARY} ${VORBIS_STATIC_LIBRARY} ${OGG_STATIC_LIBRARY})
else(WIN32)
	# FIXME: physfs
	target_link_libraries(glestshared ${SDL_LIBRARY} ${OPENAL_LIBRARY} ${VORBISFILE_LIBRARY} ${PHYSFS_LIBRARY})
endif(WIN32)
