# Install script for directory: E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/shapes

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/obj.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/obj.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/ply.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/ply.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/blender.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/blender.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/serialized.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/serialized.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/cylinder.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/cylinder.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/disk.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/disk.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/rectangle.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/rectangle.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/sdfgrid.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/sdfgrid.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/sphere.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/sphere.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/cube.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/cube.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/bsplinecurve.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/bsplinecurve.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/linearcurve.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/linearcurve.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/shapegroup.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/shapegroup.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/instance.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/instance.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/merge.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/merge.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/ellipsoids.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/ellipsoids.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/ellipsoidsmesh.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin/plugins" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/plugins/ellipsoidsmesh.dll")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/shapes/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
