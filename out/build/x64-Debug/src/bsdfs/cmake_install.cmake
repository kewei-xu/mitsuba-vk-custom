# Install script for directory: E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/bsdfs

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/blendbsdf.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/blendbsdf.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/bumpmap.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/bumpmap.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/conductor.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/conductor.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/dielectric.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/dielectric.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/diffuse.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/diffuse.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/hair.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/hair.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/mask.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/mask.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/measured.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/measured.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/normalmap.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/normalmap.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/null.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/null.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/plastic.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/plastic.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/roughconductor.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/roughconductor.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/roughdielectric.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/roughdielectric.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/roughplastic.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/roughplastic.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/thindielectric.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/thindielectric.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/twosided.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/twosided.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/polarizer.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/polarizer.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/retarder.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/retarder.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/circular.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/circular.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/measured_polarized.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/measured_polarized.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/pplastic.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/pplastic.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/principled.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/principled.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/principledthin.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/principledthin.dll")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/micrograin/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/bsdfs/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
