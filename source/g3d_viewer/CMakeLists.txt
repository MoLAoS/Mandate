# glest g3d viewer

file(GLOB viewer_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${glestshared_incs})
add_executable(g3d_viewer ${viewer_srcs})
target_link_libraries(g3d_viewer shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(g3d_viewer PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS g3d_viewer DESTINATION bin)
