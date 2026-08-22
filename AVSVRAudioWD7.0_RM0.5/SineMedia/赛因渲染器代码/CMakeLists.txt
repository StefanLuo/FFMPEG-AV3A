cmake_minimum_required(VERSION 3.5)

############################################################
# set global paths
############################################################
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include_directories(include)

############################################################
# init project
############################################################
project(adm_render VERSION 0.1.0 LANGUAGES CXX)

if(${PROJECT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    set(IS_ROOT_PROJECT TRUE)
else()
    set(IS_ROOT_PROJECT FALSE)
endif()

string(TIMESTAMP PROJECT_BUILD_DATE)
set(PROJECT_DESCRIPTION "EBU Tech 3388 core library")

############################################################
# add new CMAKE_BUILD_TYPE coverage
############################################################
SET(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
SET(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage")

############################################################
# user config options
############################################################
include(FeatureSummary)
option(BUILD_SHARED_LIBS "Build shared libraries rather than static" FALSE)
option(ADM_RENDER_HIDE_INTERNAL_SYMBOLS "Hide symbols by default" TRUE)
option(ADM_RENDER_PACKAGE_AND_INSTALL "Package and install libadmrender" ${IS_ROOT_PROJECT})
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(INSTALL_CMAKE_DIR CMake CACHE PATH "Installation directory for CMake files")
else()
  set(INSTALL_CMAKE_DIR share/cmake/adm_render CACHE PATH "Installation directory for CMake files")
endif()

############################################################
# find libraries
############################################################
find_package(Boost 1.57 REQUIRED)
include(${PROJECT_SOURCE_DIR}/resources/eigen3.cmake)

############################################################
# configure files
############################################################
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/version.hpp")

############################################################
# add targets
############################################################
add_subdirectory(resources)
add_subdirectory(src)
add_subdirectory(utils)
add_subdirectory(utils/libadm)
add_subdirectory(main/objects_render)
add_subdirectory(main/direct_speakers_render)
add_subdirectory(main/hoa_render)

############################################################
# FeatureSummary
############################################################
add_feature_info(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} "Build shared libraries rather than static")
add_feature_info(ADM_RENDER_HIDE_INTERNAL_SYMBOLS ${ADM_RENDER_HIDE_INTERNAL_SYMBOLS} "Hide symbols by default")
add_feature_info(ADM_RENDER_PACKAGE_AND_INSTALL ${ADM_RENDER_PACKAGE_AND_INSTALL} "Package and install adm_render")
feature_summary(WHAT ALL)

if(ADM_RENDER_PACKAGE_AND_INSTALL)
  #########################################################
  # very minimal cpack configuration
  #########################################################
  include(InstallRequiredSystemLibraries)
  set (CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
  set (CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
  set (CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
  set (CPACK_GENERATOR "TGZ;ZIP")
  include (CPack)

  #########################################################
  # custom uninstall target
  #########################################################
  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
      IMMEDIATE @ONLY
  )
  add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)
endif()
