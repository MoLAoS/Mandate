# glest map editor

file(GLOB map_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${shared_lib_incs})
add_executable(glest_editor ${map_srcs})
target_link_libraries(glest_editor shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})

install(TARGETS glest_editor DESTINATION bin)
