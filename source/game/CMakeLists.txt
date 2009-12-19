# glest game

set(folders
	.
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
set(game_srcs "")
foreach(folder ${folders})
	file(GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${folder}/*.cpp" "${folder}/*.h")
	include_directories(${folder})
	list(APPEND game_srcs ${srcs})
endforeach(folder)

include_directories(${shared_lib_incs})
add_executable(glestae ${game_srcs})
if(WIN32)
	target_link_libraries(glestae shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
else(WIN32)
	target_link_libraries(glestae shared_lib ${SDL_LIBRARY} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY}
							${OPENAL_LIBRARY} ${LUA_LIBRARIES} ${VORBISFILE_LIBRARY} ${ZLIB_LIBRARIES})
endif(WIN32)

install(TARGETS glestae DESTINATION bin)
