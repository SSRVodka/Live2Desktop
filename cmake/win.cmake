# Tested Compiler: MSVC 19+

macro(win_build)

# Detect compiler.
if(MSVC_VERSION MATCHES 1800)
  # Visual Studio 2013
  set(COMPILER 120)
  message(STATUS "Visual Studio 2013 detected.")
elseif(MSVC_VERSION MATCHES 1900)
  # Visual Studio 2015
  set(COMPILER 140)
  message(STATUS "Visual Studio 2015 detected.")
elseif(MSVC_VERSION GREATER_EQUAL 1910 AND MSVC_VERSION LESS 1920)
  # Visual Studio 2017
  set(COMPILER 141)
  message(STATUS "Visual Studio 2017 detected.")
elseif(MSVC_VERSION GREATER_EQUAL 1920 AND MSVC_VERSION LESS 1930)
  # Visual Studio 2019
  set(COMPILER 142)
  message(STATUS "Visual Studio 2019 detected.")
elseif(MSVC_VERSION GREATER_EQUAL 1930 AND MSVC_VERSION LESS 1940)
  # Visual Studio 2022
  set(COMPILER 143)
  message(STATUS "Visual Studio 2022 detected.")
elseif(MSVC)
  message(FATAL_ERROR "[${APP_NAME}] Unsupported Visual C++ compiler used.")
else()
  message(FATAL_ERROR "[${APP_NAME}] Unsupported compiler used.")
endif()

# Set project properties.
set_target_properties(${APP_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY
    ${PROJECT_BINARY_DIR}/bin/${APP_NAME}/${CMAKE_CFG_INTDIR}
)
if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    set_property(TARGET ${APP_NAME} PROPERTY WIN32_EXECUTABLE true)
endif()

# Add Cubism Core.
if(${BUILD_SHARED_LIBS})
    # Import as shared library.
    add_library(Live2DCubismCore SHARED IMPORTED)
    # Find library path.
    set(CORE_LIB_PREFIX ${CORE_PATH}/dll/${OS}/${ARCH})
    set_target_properties(Live2DCubismCore
      PROPERTIES
        IMPORTED_IMPLIB
        ${CORE_LIB_PREFIX}/Live2DCubismCore.lib
        IMPORTED_LOCATION
        ${CORE_LIB_PREFIX}/Live2DCubismCore.dll
        INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
    )
    # Copy shared libraries to build directory.
    add_custom_command(
        TARGET ${APP_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CORE_LIB_PREFIX}/Live2DCubismCore.dll $<TARGET_FILE_DIR:${APP_NAME}>/lib
        COMMAND ${CMAKE_COMMAND} -E copy ${CORE_LIB_PREFIX}/Live2DCubismCore.lib $<TARGET_FILE_DIR:${APP_NAME}>/lib
        COMMENT "Copying ${CORE_LIB_PREFIX}/Live2DCubismCore.{dll,lib} to destination"
    )
else()
    # Import as static library.
    add_library(Live2DCubismCore STATIC IMPORTED)
    set(CORE_LIB_PREFIX ${CORE_PATH}/lib/${OS}/${ARCH}/${COMPILER})
    set_target_properties(Live2DCubismCore
      PROPERTIES
        IMPORTED_IMPLIB
        ${CORE_LIB_PREFIX}/Live2DCubismCore_MTd.lib
        IMPORTED_LOCATION
        ${CORE_LIB_PREFIX}/Live2DCubismCore_MT.lib
        INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
    )
endif()

# Add Cubism Native Framework.
add_subdirectory(${FRAMEWORK_PATH} ${PROJECT_BINARY_DIR}/Framework)

# Link libraries to framework.
target_link_libraries(Framework Live2DCubismCore)

# Link libraries to app.
target_link_libraries(${APP_NAME}
  Framework
  # Solve the MSVCRT confliction.
  debug -NODEFAULTLIB:libcmtd.lib
  optimized -NODEFAULTLIB:libcmt.lib
)

endmacro(win_build)