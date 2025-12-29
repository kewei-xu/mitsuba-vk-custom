# Install script for directory: E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/scalar_rgb.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/scalar_spectral.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/cuda_ad_rgb.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/llvm_ad_rgb.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/llvm_ad_spectral.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/python/nanobind-drjit.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/nanobind-drjit.dll")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/mitsuba_ext.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE MODULE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/mitsuba_alias.cp311-win_amd64.pyd")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/detail.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/mitsuba_stubs" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/mitsuba_stubs/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/guiding.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/common.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/direct_projective.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/prb.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/prb_basic.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/prb_projective.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/prbvolpath.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad/integrators" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/integrators/volprim_rf_basic.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/largesteps.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/optimizers.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/ad" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/ad/projective.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/chi2.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/cli.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/math_py.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/polvis.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/sys_info.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/tensor_io.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/test" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/test/__init__.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python/test" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/test/util.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/testing.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/tonemap.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba/python" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/src/python/python/util.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE FILE FILES "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/python/../../python/mitsuba/config.py")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mitsuba" TYPE FILE FILES
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/__init__.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/py.typed"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/math.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/quad.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/spline.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/mueller.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/warp.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/misc.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/detail.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/filesystem.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/python/ad.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/python/chi2.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/python/math_py.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/python/util.pyi"
    "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/Debug/python/mitsuba/parser.pyi"
    )
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "E:/THESE/Code/mitsuba-3.7.1/mitsuba3/out/build/x64-Debug/src/python/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
