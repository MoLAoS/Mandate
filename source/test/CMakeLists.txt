# glest testsuite

set(folders
	search
	datastructs
	facilities
	.
)
# foreach(folder ${folders})
# 	file(GLOB srcs RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${folder}/*.cpp" "${folder}/*.h")
# 	list(APPEND testsuite_srcs ${srcs})
# 	# to get subfolders in VC project
# 	if(WIN32)
# 		string(REPLACE "/" "\\" folder ${folder})
# 	endif(WIN32)
# 	source_group("${folder}" FILES ${srcs})
# endforeach(folder)
include_directories(${folders})

set(test_srcs
	search/influence_map_test.cpp
	search/line_test.cpp
	main.cpp
	datastructs/circular_buffer_test.cpp
	datastructs/fixed_point_test.cpp
	datastructs/heap_test.cpp
	facilities/reverse_rect_iter_test.cpp
	search/influence_map_test.h
	search/line_test.h
	datastructs/circular_buffer_test.h
	datastructs/fixed_point_test.h
	datastructs/heap_test.h
	facilities/reverse_rect_iter_test.h
)

if(CMAKE_CXX_FLAGS MATCHES -fno-rtti)
	string(REPLACE -fno-rtti "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
endif()

include_directories(${glestshared_incs} ${game_incs} ${CPPUNIT_INCLUDE_DIR})
add_executable(test_suite ${test_srcs})

if (WIN32)
	target_link_libraries(test_suite shared_lib wsock32 optimized ${CPPUNIT_LIBRARY} debug ${CPPUNIT_DEBUG_LIB})
else(WIN32)
	target_link_libraries(test_suite shared_lib ${CPPUNIT_LIBRARY})
endif(WIN32)

# ctest
add_test(test ${CMAKE_CURRENT_BINARY_DIR}/test_suite)
