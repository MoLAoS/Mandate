# source directory

add_subdirectory(shared_lib)
add_subdirectory(game)
if(wxWidgets_FOUND)
	include(${wxWidgets_USE_FILE})

	add_subdirectory(map_editor)
	add_subdirectory(g3d_viewer)
	add_subdirectory(configurator)
else(wxWidgets_FOUND)
	message(STATUS "disabled map editor, g3d viewer and configurator")
endif(wxWidgets_FOUND)

if(CPPUNIT_FOUND)
	add_subdirectory(test)
else(CPPUNIT_FOUND)
	message(STATUS "disabled testsuite")
endif(CPPUNIT_FOUND)

if(LIBXML2_FOUND)
	add_subdirectory(glexemel)
else(LIBXML2_FOUND)
	message(STATUS "disabled glexemel")
endif(LIBXML2_FOUND)
