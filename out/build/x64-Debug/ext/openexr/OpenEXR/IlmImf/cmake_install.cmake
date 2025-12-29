# Install script for directory: E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/ext/openexr/OpenEXR/IlmImf/IlmImf-mitsuba.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/IlmImf-mitsuba.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenEXR" TYPE FILE FILES
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfForward.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfExport.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfBoxAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfCRgbaFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfChannelList.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfChannelListAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfCompressionAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDoubleAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfFloatAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfFrameBuffer.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfHeader.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfIO.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfInputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfIntAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfLineOrderAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfMatrixAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfOpaqueAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfOutputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfRgbaFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfStringAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfVecAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfHuf.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfWav.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfLut.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfArray.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfCompression.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfLineOrder.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfName.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfPixelType.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfVersion.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfXdr.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfConvert.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfPreviewImage.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfPreviewImageAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfChromaticities.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfChromaticitiesAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfKeyCode.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfKeyCodeAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTimeCode.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTimeCodeAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfRational.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfRationalAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfFramesPerSecond.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfStandardAttributes.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfStdIO.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfEnvmap.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfEnvmapAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfInt64.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfRgba.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTileDescription.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTileDescriptionAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTiledInputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTiledOutputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTiledRgbaFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfRgbaYca.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTestFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfThreading.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfB44Compressor.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfStringVectorAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfMultiView.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfAcesFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfMultiPartOutputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfGenericOutputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfMultiPartInputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfGenericInputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfPartType.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfPartHelper.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfOutputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTiledOutputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfInputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfTiledInputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepScanLineOutputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepScanLineOutputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepScanLineInputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepScanLineInputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepTiledInputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepTiledInputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepTiledOutputFile.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepTiledOutputPart.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepFrameBuffer.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepCompositing.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfCompositeDeepScanLine.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfNamespace.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepImageState.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfDeepImageStateAttribute.h"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/ext/openexr/OpenEXR/IlmImf/ImfFloatVectorAttribute.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/ext/zlib/zlib.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/zlib1.dll")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/ext/openexr/OpenEXR/IlmImf/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
