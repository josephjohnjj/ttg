#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MADworld" for configuration "Release"
set_property(TARGET MADworld APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(MADworld PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libMADworld.so"
  IMPORTED_SONAME_RELEASE "libMADworld.so"
  )

list(APPEND _IMPORT_CHECK_TARGETS MADworld )
list(APPEND _IMPORT_CHECK_FILES_FOR_MADworld "${_IMPORT_PREFIX}/lib/libMADworld.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
