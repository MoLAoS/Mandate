# glest configurator

file(GLOB cfg_srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cpp" "*.h")

include_directories(${shared_lib_incs})
add_executable(glest_configurator ${cfg_srcs})
target_link_libraries(glest_configurator shared_lib ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${wxWidgets_LIBRARIES})

install(TARGETS glest_configurator DESTINATION bin)
