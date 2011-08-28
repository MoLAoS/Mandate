# glest g3d viewer

file(GLOB viewer_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")
if(WIN32)
	list(APPEND viewer_srcs "resources.rc")
endif(WIN32)

include_directories(${glestshared_incs})
add_executable(gae_g3dviewer ${viewer_srcs})
target_link_libraries(gae_g3dviewer shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(gae_g3dviewer PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS gae_g3dviewer DESTINATION bin)
if(UNIX AND NOT APPLE)
	install(FILES gae_g3dviewer.png DESTINATION share/pixmaps)
	install(FILES gae_g3dviewer.desktop DESTINATION share/applications)
endif(UNIX AND NOT APPLE)

if(CPPCHECK_FOUND)
	add_cppcheck(gae_g3dviewer)
endif(CPPCHECK_FOUND)
