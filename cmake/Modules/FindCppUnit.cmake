# Find CPPUnit
#
#  CPPUNIT_INCLUDE_DIR - where to find Test.h, etc.
#  CPPUNIT_LIBRARY     - path to library
#  CPPUNIT_FOUND       - true if cppunit found.

#TODO This should be just a generic extra path, CPPUNIT_BASE or somesuch
# then it would be defined in the root dir CMakeFiles.txt with all the other WINDEPS stuff
if(DEFINED WINDEPS)
	set(path_hints ${WINDEPS} $ENV{CPPUNITDIR})
else(DEFINED WINDEPS)
	set(path_hints $ENV{CPPUNITDIR})
endif(DEFINED WINDEPS)

find_path(CPPUNIT_INCLUDE_DIR cppunit/Test.h
	HINTS ${path_hints}
	PATH_SUFFIXES include
)

find_library(CPPUNIT_LIBRARY cppunit
	HINTS ${path_hints}
	PATH_SUFFIXES lib64 lib
)

# Handle the QUIETLY and REQUIRED arguments and set CPPUNIT_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CPPUNIT DEFAULT_MSG
	CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY)

mark_as_advanced(CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY)
