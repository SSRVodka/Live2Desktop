cmake_minimum_required(VERSION 3.16)

# -------------- Prebuilt-configurations --------------

# Set app name.
set(APP_NAME Live2Desktop)
# Set directory paths.
set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_PATH ${ROOT_PATH}/Core)
set(FRAMEWORK_PATH ${ROOT_PATH}/Framework)
set(THIRD_PARTY_PATH ${ROOT_PATH}/thirdParty)
set(STB_PATH ${THIRD_PARTY_PATH}/stb)
set(RES_PATH ${ROOT_PATH}/Resources)

# Set project.
project(${APP_NAME})

# Define output directory: ${ROOT_PATH}/<-B param>/bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${APP_NAME})

# Specify version of compiler.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "i386")
  set(ARCH "x86")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
  set(ARCH "x86_64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "AMD64")
  set(ARCH "x86_64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(ARCH "arm64")
else()
  message(WARNING "Untested architecture detected (${CMAKE_HOST_SYSTEM_PROCESSOR}). Use x86_64 libraries instead.")
  set(ARCH "x86_64")
endif()

message(STATUS "Target architecture: ${ARCH}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS "linux")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(OS "windows")
else()
  set(OS "macos")
endif()

message(STATUS "Target OS: ${OS}")

if(DEBUG)
 set(CMAKE_BUILD_TYPE "Debug")
else()
  set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "Target build type: ${CMAKE_BUILD_TYPE}")

# Use shared libraries or static libraries
if(USE_SHARED_LIB)
  message(STATUS "Target libraries type: shared")
else()
  message(STATUS "Taregt libraries type: static")
endif()

set(FRAMEWORK_SOURCE "OpenGL")

# -------------- Build App --------------

# Find Qt libraries
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)

# Make executable app.
add_executable(${APP_NAME})
# Add source files.
set(FORM_SRC
  ui/mainWindow.ui
  ui/configDialog.ui
)

set(QCC_QRC
  resources.qrc
)
set(MOC_HEADERS
  src/animeWidget.h
  src/configDialog.h
)

set(MAIN_SRC
  src/main.cpp

  icon.rc

  src/allocator.cpp
  src/animeWidget.cpp
  src/cJSON.c
  src/configDialog.cpp
  src/coreManager.cpp
  src/eventHandler.cpp
  src/logger.cpp
  src/mainWindow.cpp
  src/model.cpp
  src/modelManager.cpp
  src/modelParameters.cpp
  src/renderer.cpp
  src/resourceLoader.cpp
  src/textureManager.cpp
  src/tools.cpp
  src/wavFileHandler.cpp
)
set(MAIN_HEADERS
  src/allocator.h
  src/cJSON.h
  src/consts.h
  src/coreManager.h
  src/eventHandler.h
  src/logger.h
  src/mainWindow.h
  src/model.h
  src/modelManager.h
  src/modelParameters.h
  src/renderer.h
  src/resourceLoader.h
  src/textureManager.h
  src/tools.h
  src/wavFileHandler.h
)

QT5_WRAP_CPP(MOCd_HEADERS ${MOC_HEADERS})
QT5_WRAP_UI(UICd_FORMS ${FORM_SRC})
QT5_ADD_RESOURCES(RCCd_SRC ${QCC_QRC})

# Include the intermediate headers.
target_include_directories(${APP_NAME} PUBLIC ${CMAKE_BINARY_DIR})
target_include_directories(${APP_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/)

target_sources(${APP_NAME}
  PUBLIC
    ${MAIN_SRC}
    ${MOCd_HEADERS}
    ${UICd_FORMS}
    ${MOC_HEADERS}
    ${MAIN_HEADERS}
    ${RCCd_SRC}
)

# Add thirdParty/stb_image.h
target_include_directories(${APP_NAME} PRIVATE ${STB_PATH})

# Link Qt libraries
target_link_libraries(${APP_NAME}
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::OpenGL
)

# Build in multi-process.
target_compile_options(${APP_NAME} PRIVATE /MP)

# Copy resource directory to build directory.
add_custom_command(
  TARGET ${APP_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E
      copy_directory ${RES_PATH} $<TARGET_FILE_DIR:${APP_NAME}>/Resources
)

# Build Framework & Core at specific platform AND link.
if(${OS} STREQUAL "windows")
  include(cmake/win.cmake)
  win_build()
else()
  include(cmake/unix.cmake)
  unix_build()
endif()
