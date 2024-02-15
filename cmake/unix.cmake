macro(unix_build)

# support gdb
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
endif()

# Add Cubism Core.
if(USE_SHARED_LIB)
    # Import as shared library.
    add_library(Live2DCubismCore SHARED IMPORTED)
    # Find library path.
    set(CORE_LIB_PREFIX ${CORE_PATH}/dll/${OS}/${ARCH})
    if(${OS} STREQUAL "linux")
        set(DLL_NAME "libLive2DCubismCore.so")
    else()
        set(DLL_NAME "libLive2DCubismCore.dylib")
    endif()
    set_target_properties(Live2DCubismCore
      PROPERTIES
        IMPORTED_LOCATION ${CORE_LIB_PREFIX}/${DLL_NAME}
        INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
    )
else()
    # Import as static library.
    add_library(Live2DCubismCore STATIC IMPORTED)
    set(CORE_LIB_PREFIX ${CORE_PATH}/lib/${OS}/${ARCH})
    set_target_properties(Live2DCubismCore
      PROPERTIES
        IMPORTED_LOCATION ${CORE_LIB_PREFIX}/libLive2DCubismCore.a
        INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
    )
endif()


# Add Cubism Native Framework.
add_subdirectory(${FRAMEWORK_PATH} ${PROJECT_BINARY_DIR}/Framework)
# Add rendering definition to framework.
target_compile_definitions(Framework PUBLIC CSM_TARGET_LINUX_GL)

# Link libraries to framework.
target_link_libraries(Framework Live2DCubismCore)


# Link libraries to app.
target_link_libraries(${APP_NAME}
  Framework
  ${CMAKE_DL_LIBS}
  X11
  pthread
)

endmacro(unix_build)