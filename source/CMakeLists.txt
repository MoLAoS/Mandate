# source directory

add_subdirectory(shared_lib)
add_subdirectory(game)
if(wxWidgets_FOUND)
	include(${wxWidgets_USE_FILE})

	add_subdirectory(map_editor)
	add_subdirectory(g3d_viewer)
	add_subdirectory(configurator)
endif(wxWidgets_FOUND)
