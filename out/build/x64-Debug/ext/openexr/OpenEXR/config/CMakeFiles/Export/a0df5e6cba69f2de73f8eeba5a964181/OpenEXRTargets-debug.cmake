#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpenEXR::IlmImf" for configuration "Debug"
set_property(TARGET OpenEXR::IlmImf APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenEXR::IlmImf PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/IlmImf-mitsuba.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG "OpenEXR::zlib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/IlmImf-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets OpenEXR::IlmImf )
list(APPEND _cmake_import_check_files_for_OpenEXR::IlmImf "${_IMPORT_PREFIX}/lib/IlmImf-mitsuba.lib" "${_IMPORT_PREFIX}/bin/IlmImf-mitsuba.dll" )

# Import target "OpenEXR::zlib" for configuration "Debug"
set_property(TARGET OpenEXR::zlib APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenEXR::zlib PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/zlib.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/zlib1.dll"
  )

list(APPEND _cmake_import_check_targets OpenEXR::zlib )
list(APPEND _cmake_import_check_files_for_OpenEXR::zlib "${_IMPORT_PREFIX}/lib/zlib.lib" "${_IMPORT_PREFIX}/bin/zlib1.dll" )

# Import target "OpenEXR::IlmImfUtil" for configuration "Debug"
set_property(TARGET OpenEXR::IlmImfUtil APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(OpenEXR::IlmImfUtil PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/IlmImfUtil-mitsuba.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/IlmImfUtil-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets OpenEXR::IlmImfUtil )
list(APPEND _cmake_import_check_files_for_OpenEXR::IlmImfUtil "${_IMPORT_PREFIX}/lib/IlmImfUtil-mitsuba.lib" "${_IMPORT_PREFIX}/bin/IlmImfUtil-mitsuba.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
