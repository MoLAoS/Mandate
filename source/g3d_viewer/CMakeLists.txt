# glest g3d viewer

file(GLOB viewer_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${glestshared_incs} ${PHYSFS_INCLUDE_DIR})
add_executable(glest_g3dviewer ${viewer_srcs})
target_link_libraries(glest_g3dviewer glestshared ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES}
						${PHYSFS_LIBRARY})
set_target_properties(glest_g3dviewer PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS glest_g3dviewer DESTINATION bin)
