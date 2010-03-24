# glexemel

add_executable(g2xml g2xml.c g3dv4.h)

include_directories(${LIBXML2_INCLUDE_DIR})
add_executable(xml2g xml2g.c g3dv4.h)
target_link_libraries(xml2g ${LIBXML2_LIBRARIES})
