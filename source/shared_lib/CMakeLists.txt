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

if(WIN32)
	# add glprocs.c needed on windows
	list(APPEND glestshared_srcs ${GLPROCS_FILE})
	
	# pre-compiled headers...
	set(pch "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h" "${CMAKE_CURRENT_SOURCE_DIR}/sources/pch.cpp")
	list(APPEND glestshared_srcs "${pch}")
	source_group("" FILES ${GLPROCS_FILE} ${pch})
endif(WIN32)

# ugly, but otherwise not the updated stuff
set(glestshared_incs ${glestshared_incs} PARENT_SCOPE)

include_directories(${glestshared_incs})
add_library(glestshared STATIC ${glestshared_srcs})
target_link_libraries(glestshared ${LUA_LIBRARIES} ${PHYSFS_LIBRARY} ${ZLIB_LIBRARIES})
if(WIN32)
	# FIXME: VC project needs static vorbis/ogg libs
	target_link_libraries(glestshared ${VORBISFILE_STATIC_LIBRARY} ${VORBIS_STATIC_LIBRARY} ${OGG_STATIC_LIBRARY})
else(WIN32)
	target_link_libraries(glestshared ${SDL_LIBRARY} ${OPENAL_LIBRARY} ${VORBISFILE_LIBRARY})
endif(WIN32)

if (WIN32)
	# set linker flags for Release to use link time code gen.
	set_target_properties(glestshared PROPERTIES LINK_FLAGS_RELEASE "/LTCG")

	# set-up precompiled headers (for windows)
	set_target_properties(glestshared PROPERTIES COMPILE_FLAGS "/Yu\"pch.h\" /DUSE_PCH=1")
	set_source_files_properties(sources/pch.cpp PROPERTIES COMPILE_FLAGS "/Yc\"pch.h\"")
	set_source_files_properties(${GLPROCS_FILE} PROPERTIES COMPILE_FLAGS "/Y-")
endif(WIN32)
