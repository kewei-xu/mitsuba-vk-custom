#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "IlmBase::Half" for configuration "Debug"
set_property(TARGET IlmBase::Half APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(IlmBase::Half PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/Half-mitsuba.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/Half-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets IlmBase::Half )
list(APPEND _cmake_import_check_files_for_IlmBase::Half "${_IMPORT_PREFIX}/lib/Half-mitsuba.lib" "${_IMPORT_PREFIX}/bin/Half-mitsuba.dll" )

# Import target "IlmBase::Iex" for configuration "Debug"
set_property(TARGET IlmBase::Iex APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(IlmBase::Iex PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/Iex-mitsuba.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/Iex-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets IlmBase::Iex )
list(APPEND _cmake_import_check_files_for_IlmBase::Iex "${_IMPORT_PREFIX}/lib/Iex-mitsuba.lib" "${_IMPORT_PREFIX}/bin/Iex-mitsuba.dll" )

# Import target "IlmBase::IexMath" for configuration "Debug"
set_property(TARGET IlmBase::IexMath APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(IlmBase::IexMath PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/IexMath-mitsuba.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/IexMath-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets IlmBase::IexMath )
list(APPEND _cmake_import_check_files_for_IlmBase::IexMath "${_IMPORT_PREFIX}/lib/IexMath-mitsuba.lib" "${_IMPORT_PREFIX}/bin/IexMath-mitsuba.dll" )

# Import target "IlmBase::Imath" for configuration "Debug"
set_property(TARGET IlmBase::Imath APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(IlmBase::Imath PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/Imath-mitsuba.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/Imath-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets IlmBase::Imath )
list(APPEND _cmake_import_check_files_for_IlmBase::Imath "${_IMPORT_PREFIX}/lib/Imath-mitsuba.lib" "${_IMPORT_PREFIX}/bin/Imath-mitsuba.dll" )

# Import target "IlmBase::IlmThread" for configuration "Debug"
set_property(TARGET IlmBase::IlmThread APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(IlmBase::IlmThread PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/IlmThread-mitsuba.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/IlmThread-mitsuba.dll"
  )

list(APPEND _cmake_import_check_targets IlmBase::IlmThread )
list(APPEND _cmake_import_check_files_for_IlmBase::IlmThread "${_IMPORT_PREFIX}/lib/IlmThread-mitsuba.lib" "${_IMPORT_PREFIX}/bin/IlmThread-mitsuba.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
