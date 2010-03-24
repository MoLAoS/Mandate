# Find CPPUnit
#
#  CPPUNIT_INCLUDE_DIR - where to find Test.h, etc.
#  CPPUNIT_LIBRARY     - path to library
#  CPPUNIT_FOUND       - true if cppunit found.

find_path(CPPUNIT_INCLUDE_DIR cppunit/Test.h
	HINTS $ENV{CPPUNITDIR}
	PATH_SUFFIXES include
)

find_library(CPPUNIT_LIBRARY cppunit
	HINTS $ENV{CPPUNITDIR}
	PATH_SUFFIXES lib64 lib
)

# Handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CPPUNIT DEFAULT_MSG
	CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY)

mark_as_advanced(CPPUNIT_INCLUDE_DIR CPPUNIT_LIBRARY)
