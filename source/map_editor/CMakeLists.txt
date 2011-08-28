# glest map editor

file(GLOB map_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
if(WIN32)
	list(APPEND map_srcs "resources.rc")
endif(WIN32)

include_directories(${glestshared_incs})
add_executable(gae_mapeditor ${map_srcs})
target_link_libraries(gae_mapeditor shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(gae_mapeditor PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS gae_mapeditor DESTINATION bin)
if(UNIX AND NOT APPLE)
	install(FILES gae_mapeditor.png DESTINATION share/pixmaps)
	install(FILES gae_mapeditor.desktop DESTINATION share/applications)
endif(UNIX AND NOT APPLE)

if(CPPCHECK_FOUND)
	add_cppcheck(gae_mapeditor)
endif(CPPCHECK_FOUND)
