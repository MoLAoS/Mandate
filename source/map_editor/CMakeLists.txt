# glest map editor

file(GLOB map_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${glestshared_incs})
add_executable(glest_editor ${map_srcs})
target_link_libraries(glest_editor glestshared ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(glest_editor PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS glest_editor DESTINATION bin)
