# glest game

set(folders
	.
	ai
	ai_mandate
	creatable_types
	entities
	facilities
	game
	global
	graphics
	gui
	gui_mandate
	hierarchy
	html_interface
	main
	menu
	network
	prototypes
	Rocket
	script
	search
	sound
	widgets
	world
)
#file(GLOB game_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
#source_group("" FILES ${game_srcs})  # . now in folders
foreach(folder ${folders})
	file(GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${folder}/*.cpp" "${folder}/*.h")

# 	include_directories(${folder})
	list(APPEND game_incs "${CMAKE_CURRENT_SOURCE_DIR}/${folder}")
	list(APPEND game_srcs ${srcs})
	# to get subfolders in VC project
	source_group(${folder} FILES ${srcs})
endforeach(folder)
if(WIN32)
	list(APPEND game_srcs "resources.rc")
	source_group("." FILES "resources.rc")
endif(WIN32)

# create version file
if(WIN32)
	add_custom_command(
		OUTPUT version.cpp
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version.bat
		DEPENDS glestadv
	)
else(WIN32)
	add_custom_command(
		OUTPUT version.cpp
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/version.sh ${CMAKE_SOURCE_DIR}
		#DEPENDS glestadv
	)
	add_custom_target(version ALL DEPENDS version.cpp)
endif(WIN32)

#list(APPEND game_incs "version.h")
list(APPEND game_srcs "version.cpp")

# ugly, but otherwise not the updated stuff
set(game_incs ${game_incs} PARENT_SCOPE)

include_directories(${game_incs} ${glestshared_incs}) # order important because different pch.h in both

add_executable(glestadv ${game_srcs})

target_link_libraries(glestadv shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY})
if(WIN32)
	target_link_libraries(glestadv ${DXGUID_LIBRARY} ${DSOUND_LIBRARY} wsock32)
else(WIN32)
	target_link_libraries(glestadv ${SDL_LIBRARY} ${OPENAL_LIBRARY})
endif(WIN32)

install(TARGETS glestadv DESTINATION bin)
if(UNIX AND NOT APPLE)
	install(FILES glestadv.png DESTINATION share/pixmaps)
	install(FILES glestadv.desktop DESTINATION share/applications)
endif(UNIX AND NOT APPLE)

if (WIN32)
	# set linker flags for Release to use link time code gen, and set up some debug output for stack-traces.
	set_target_properties(glestadv PROPERTIES LINK_FLAGS_RELEASE "/LTCG /DEBUG /MAP")

	if (GAE_USE_PRECOMPILED_HDR)
		# set-up precompiled headers (for windows)
		# project wide setting
		set_target_properties(glestadv PROPERTIES COMPILE_FLAGS "/Yu\"pch.h\" /DUSE_PCH=1")
		# create through pch.cpp
		set_source_files_properties(./pch.cpp PROPERTIES COMPILE_FLAGS "/Yc\"pch.h\"")
		# game_constants.cpp must not use precompiled header (then we'd get no EnumNames objects)
		set_source_files_properties(game/game_constants.cpp PROPERTIES COMPILE_FLAGS "/Y-")
	else (GAE_USE_PRECOMPILED_HDR)
		set_target_properties(glestadv PROPERTIES COMPILE_FLAGS "/DUSE_PCH=0")
	endif (GAE_USE_PRECOMPILED_HDR)
endif (WIN32)

if(CPPCHECK_FOUND)
	add_cppcheck(glestadv)
endif(CPPCHECK_FOUND)
