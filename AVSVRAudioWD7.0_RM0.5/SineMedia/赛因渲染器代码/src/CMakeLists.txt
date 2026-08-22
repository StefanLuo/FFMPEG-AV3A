# --- render library ---

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/resources.hpp
  COMMAND adm_render_resource_embedder Quadrature.dat > ${PROJECT_BINARY_DIR}/resources.hpp
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/resources
  DEPENDS adm_render_resource_embedder
      ${PROJECT_SOURCE_DIR}/resources/Quadrature.dat
  COMMENT "Running resource embedder."
  VERBATIM
)

add_library(render
  bs2051.cpp
  bs2051_layouts.cpp
  common/facets.cpp
  common/geom.cpp
  common/point_source_panner.cpp
  decorrelate.cpp
  direct_speakers/gain_calculator_direct_speakers.cpp
  direct_speakers/mapping_rules.cpp
  dsp/block_convolver.cpp
  dsp/block_convolver_impl.cpp
  dsp/delay_buffer_impl.cpp
  dsp/variable_block_size_impl.cpp
  gain_calculators.cpp
  fft_kiss.cpp
  hoa/gain_calculator_hoa.cpp
  hoa/hoa.cpp
  layout.cpp
  screen.cpp
  object_based/extent.cpp
  object_based/gain_calculator_objects.cpp
  warnings.cpp
  ${PROJECT_BINARY_DIR}/resources.hpp
)

target_include_directories(render
  PUBLIC
  # Headers used from source/build location:
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/resources> # resources
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>  # config.h / export.h
  # Headers used from installed location:
    $<INSTALL_INTERFACE:${INSTALL_INCLUDE_DIR}>
)

target_link_libraries(render PUBLIC Boost::boost ${CMAKE_DL_LIBS})
# Only use include directory of Eigen3 target
# as otherwise Eigen3 needs to be installed and found when linking to a
# static version of librender, even though it's an INTERFACE target with PRIVATE linking
# This is because cmake doesn't check if the INTERFACE target itself might have linker dependencies
# or not (see https://gitlab.kitware.com/cmake/cmake/issues/18049)
if(render_HIDE_INTERNAL_SYMBOLS)
    target_include_directories(render PRIVATE $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>)
else()
    target_include_directories(render PUBLIC $<TARGET_PROPERTY:Eigen3::Eigen,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

set_property(TARGET render PROPERTY POSITION_INDEPENDENT_CODE ON)

if(ADM_RENDER_HIDE_INTERNAL_SYMBOLS AND BUILD_SHARED_LIBS)
  set_target_properties(render PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN TRUE)
endif()

############################################################
# enable C++11 support
############################################################
if ({CMAKE_VERSION} VERSION_LESS "3.8.0")
  # Note: this is not a complete list of c++ features required by librender.
  # What we want is C++11 support and this is a simple way to trigger
  # this for CMake < 3.8
  target_compile_features(render PUBLIC cxx_auto_type cxx_nullptr cxx_range_for)
else()
  target_compile_features(render PUBLIC cxx_std_11)
endif()
set_target_properties(render PROPERTIES CXX_EXTENSIONS OFF)

include(GenerateExportHeader)
generate_export_header(render
  EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/generated/export.hpp
)

############################################################
# export package
############################################################
if(render_PACKAGE_AND_INSTALL)
  include(CMakePackageConfigHelpers)

  configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/config/renderConfig.cmake.in
    ${PROJECT_BINARY_DIR}/renderConfig.cmake
    INSTALL_DESTINATION
    ${INSTALL_CMAKE_DIR}
    PATH_VARS INSTALL_INCLUDE_DIR INSTALL_LIB_DIR INSTALL_CMAKE_DIR
  )

  write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/renderConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
  )

  ############################################################
  # install render library
  ############################################################
  set_target_properties(render PROPERTIES
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
  )

  install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/render/
    DESTINATION ${INSTALL_INCLUDE_DIR}/render
  )
  install(DIRECTORY ${PROJECT_BINARY_DIR}/generated/
    DESTINATION ${INSTALL_INCLUDE_DIR}/render/generated
  )

  install(FILES
    ${PROJECT_BINARY_DIR}/renderConfig.cmake
    ${PROJECT_BINARY_DIR}/renderConfigVersion.cmake
    DESTINATION ${INSTALL_CMAKE_DIR}
  )

  if(${BUILD_SHARED_LIBS})
    install(TARGETS render
      EXPORT renderTargetsShared
      LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
      RUNTIME DESTINATION "${INSTALL_LIB_DIR}"
      ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
      PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}"
    )
    install(EXPORT renderTargetsShared DESTINATION ${INSTALL_CMAKE_DIR})
  else()
    install(TARGETS render
      EXPORT renderTargetsStatic
      LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
      RUNTIME DESTINATION "${INSTALL_LIB_DIR}"
      ARCHIVE DESTINATION "${INSTALL_LIB_DIR}"
      PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}"
    )
    install(EXPORT renderTargetsStatic DESTINATION ${INSTALL_CMAKE_DIR})
  endif()
endif()
