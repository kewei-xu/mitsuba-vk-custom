# Install script for directory: E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/ext/openexr/IlmBase/Imath/Imath-mitsuba.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/Imath-mitsuba.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenEXR" TYPE FILE FILES
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathBoxAlgo.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathBox.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathColorAlgo.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathColor.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathEuler.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathExc.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathExport.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathForward.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathFrame.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathFrustum.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathFrustumTest.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathFun.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathGL.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathGLU.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathHalfLimits.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathInt64.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathInterval.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathLimits.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathLineAlgo.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathLine.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathMath.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathMatrixAlgo.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathMatrix.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathNamespace.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathPlane.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathPlatform.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathQuat.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathRandom.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathRoots.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathShear.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathSphere.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathVecAlgo.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/IlmBase/Imath/ImathVec.h"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/ext/openexr/IlmBase/Imath/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
