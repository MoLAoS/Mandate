# glest g3d viewer

file(GLOB viewer_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${shared_lib_incs})
add_executable(glest_g3dviewer ${viewer_srcs})
target_link_libraries(glest_g3dviewer shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(glest_g3dviewer PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS glest_g3dviewer DESTINATION bin)
