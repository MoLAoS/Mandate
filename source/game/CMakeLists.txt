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
add_executable(glestadv ${game_srcs})
target_link_libraries(glestadv shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY}
					${LUA_LIBRARIES} ${ZLIB_LIBRARIES})
if(WIN32)
	# FIXME: VC project needs static vorbis/ogg libs
	target_link_libraries(glestadv ${DXGUID_LIBRARY}
					${VORBISFILE_STATIC_LIBRARY} ${VORBIS_STATIC_LIBRARY} ${OGG_STATIC_LIBRARY})
else(WIN32)
	target_link_libraries(glestadv ${SDL_LIBRARY} ${OPENAL_LIBRARY} ${VORBISFILE_LIBRARY})
endif(WIN32)

install(TARGETS glestadv DESTINATION bin)
