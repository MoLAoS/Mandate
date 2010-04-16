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

include_directories(${glestshared_incs} ${game_incs} ${CPPUNIT_INCLUDE_DIR})
add_executable(testsuite ${test_srcs})
target_link_libraries(testsuite glestshared ${CPPUNIT_LIBRARY} ${PHYSFS_LIBRARY})
if (WIN32)
	target_link_libraries(testsuite wsock32)
endif(WIN32)
