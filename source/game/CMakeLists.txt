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
	include_directories(${folder})
	list(APPEND game_srcs ${srcs})
	# to get subfolders in VC project
	source_group(${folder} FILES ${srcs})
endforeach(folder)

include_directories(${glestshared_incs})
add_executable(glestadv ${game_srcs})
target_link_libraries(glestadv glestshared ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${ZLIB_LIBRARIES})
if(WIN32)
	target_link_libraries(glestadv ${DXGUID_LIBRARY} ${DSOUND_LIBRARY} wsock32)
else(WIN32)
	target_link_libraries(glestadv ${SDL_LIBRARY} ${OPENAL_LIBRARY})
endif(WIN32)

install(TARGETS glestadv DESTINATION bin)
