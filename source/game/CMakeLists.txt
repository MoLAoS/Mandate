# glest game

set(folders
	ai
	facilities
	game
	global
	graphics
	gui
	main
	menu
	network
	path_finder
	sound
	type_instances
	types
	world
)
file(GLOB game_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
source_group("" FILES ${game_srcs})
foreach(folder ${folders})
	file(GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${folder}/*.cpp" "${folder}/*.h")
# 	include_directories(${folder})
	list(APPEND game_incs "${CMAKE_CURRENT_SOURCE_DIR}/${folder}")
	list(APPEND game_srcs ${srcs})
	# to get subfolders in VC project
	source_group(${folder} FILES ${srcs})
endforeach(folder)

# ugly, but otherwise not the updated stuff
set(game_incs ${game_incs} PARENT_SCOPE)

include_directories(${glestshared_incs} ${game_incs})
add_executable(glestadv ${game_srcs})
target_link_libraries(glestadv glestshared ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
if(WIN32)
	target_link_libraries(glestadv ${DXGUID_LIBRARY} ${DSOUND_LIBRARY} wsock32)
else(WIN32)
	target_link_libraries(glestadv ${SDL_LIBRARY} ${OPENAL_LIBRARY})
endif(WIN32)

install(TARGETS glestadv DESTINATION bin)

if (WIN32)
	# set linker flags for Release to use link time code gen.
	set_target_properties(glestadv PROPERTIES LINK_FLAGS_RELEASE "/LTCG")

	# set-up precompiled headers (for windows)
	# project wide setting
	set_target_properties(glestadv PROPERTIES COMPILE_FLAGS "/Yu\"pch.h\" /DUSE_PCH=1")
	# create through pch.cpp
	set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Yc\"pch.h\"")
	# game_constants.cpp must not use precompiled header (then we'd get no EnumNames objects)
	set_source_files_properties(game/game_constants.cpp PROPERTIES COMPILE_FLAGS "/Y-")
	
endif(WIN32)

