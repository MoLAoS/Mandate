# glest configurator

file(GLOB cfg_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${glestshared_incs})
add_executable(glest_configurator ${cfg_srcs})
target_link_libraries(glest_configurator glestshared ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})
set_target_properties(glest_configurator PROPERTIES WIN32_EXECUTABLE "true")

install(TARGETS glest_configurator DESTINATION bin)
