# blendnorm - blendmap compositor & normaliser

add_executable(blendnorm blendnorm.cpp)

include_directories(${glestshared_incs})
target_link_libraries(blendnorm shared_lib)
