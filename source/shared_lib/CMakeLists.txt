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
	if (GAE_USE_XAUDIO2)
		message("Using XAudio.")
		list(APPEND folders platform/win32 sound/xa2)
	endif (GAE_USE_XAUDIO2)
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

if (MSVC)
	# pre-compiled headers...
	set(pch "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h" "${CMAKE_CURRENT_SOURCE_DIR}/sources/pch.cpp")
	list(APPEND glestshared_srcs "${pch}")
	source_group("" FILES ${GLPROCS_FILE} ${pch})
endif(MSVC)

# ugly, but otherwise not the updated stuff
set(glestshared_incs ${glestshared_incs} PARENT_SCOPE)

include_directories(${glestshared_incs})

add_library(shared_lib STATIC ${glestshared_srcs})

#message("FREETYPE_LIBRARY == " + ${FREETYPE_LIBRARY})
#message("FREETYPE_LIBRARIES == " + ${FREETYPE_LIBRARIES})
target_link_libraries(shared_lib
	${LUA_LIBRARIES}
	${ZLIB_LIBRARIES}
	${FREETYPE_LIBRARIES}
	${PNG_LIBRARY}
	${JPEG_LIBRARY}
	${GLEW_LIBRARY}
	${PHYSFS_LIBRARY})

if(WIN32)
	# FIXME: VC project needs static vorbis/ogg libs
	target_link_libraries(shared_lib 
		${VORBISFILE_STATIC_LIBRARY}
		${VORBIS_STATIC_LIBRARY}
		${OGG_STATIC_LIBRARY})
else(WIN32)
	target_link_libraries(shared_lib
		${SDL_LIBRARY}
		${OPENAL_LIBRARY}
		${VORBISFILE_LIBRARY})
endif(WIN32)

if (MSVC)
	# set linker flags for Release to use link time code gen.
	set_target_properties(shared_lib PROPERTIES LINK_FLAGS_RELEASE "/LTCG")

	if (GAE_USE_PRECOMPILED_HDR)
		# set-up precompiled headers (for windows)
		# project wide,
		set_target_properties(shared_lib PROPERTIES COMPILE_FLAGS "/Yu\"pch.h\" /DUSE_PCH=1")
		# create through pch.cpp
		set_source_files_properties(sources/pch.cpp PROPERTIES COMPILE_FLAGS "/Yc\"pch.h\"")
		# but not for glprocs.c
		set_source_files_properties(${GLPROCS_FILE} PROPERTIES COMPILE_FLAGS "/Y-")
	else (GAE_USE_PRECOMPILED_HDR)
		set_target_properties(shared_lib PROPERTIES COMPILE_FLAGS "/DUSE_PCH=0")
	endif (GAE_USE_PRECOMPILED_HDR)

endif(MSVC)

if(CPPCHECK_FOUND)
	add_cppcheck(shared_lib)
endif(CPPCHECK_FOUND)
