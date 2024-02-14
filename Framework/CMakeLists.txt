cmake_minimum_required(VERSION 3.10)

set(LIB_NAME Framework)

# Find Qt libraries
find_package(Qt5 COMPONENTS OpenGL REQUIRED)

# Force static library.
add_library(${LIB_NAME} STATIC)

target_sources(${LIB_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismCdiJson.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismCdiJson.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismDefaultParameterId.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismDefaultParameterId.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismFramework.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismFramework.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismFrameworkConfig.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismModelSettingJson.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismModelSettingJson.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CubismJsonHolder.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ICubismAllocator.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ICubismModelSetting.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Live2DCubismCore.hpp

    ${CMAKE_CURRENT_SOURCE_DIR}/AppOpenGLWrapper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AppOpenGLWrapper.cpp
)

# Add sub directories.
add_subdirectory(Effect)
add_subdirectory(Id)
add_subdirectory(Math)
add_subdirectory(Model)
add_subdirectory(Motion)
add_subdirectory(Physics)
add_subdirectory(Rendering)
add_subdirectory(Type)
add_subdirectory(Utils)


# Add include path.
target_include_directories(${LIB_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${RENDER_INCLUDE_PATH}
)

# Deprecated functions
# The following expressions are written for compatibility
# and will be removed in a future release.

# Add core include.
target_include_directories(${LIB_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/include
)
# Add definitions.
target_compile_definitions(${LIB_NAME}
  PRIVATE
    ${FRAMEWORK_DEFINITIOINS}
)

# Link Qt libraries
target_link_libraries(${LIB_NAME}
  Qt5::OpenGL
)
