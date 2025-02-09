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

# Find Qt libraries
find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

# Define Qt5 Process Functions for UIC (specific output headers)
# Output[output_form_headers]: Headers file list
# Input[form_src]: Form source list
# Input[output_dir]: Compiled output directory
function(QT5_WRAP_UI_SPEC output_form_headers form_src output_dir)
  QT5_WRAP_UI(COMPILED_FORMS ${form_src})
  # Override the output directory for the generated files
  foreach(GENERATED_FORM ${COMPILED_FORMS})
    # Get the file name of the generated header
    get_filename_component(FORM_FILENAME ${GENERATED_FORM} NAME)
    # Create the target path in the custom directory
    set(FORM_NEW_PATH ${output_dir}/${FORM_FILENAME})
    # Add a custom command to move the generated file to the custom directory
    add_custom_command(
        OUTPUT ${FORM_NEW_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GENERATED_FORM} ${FORM_NEW_PATH}
        DEPENDS ${GENERATED_FORM}
        COMMENT "Moving ${GENERATED_FORM} to ${FORM_NEW_PATH}"
    )
    # Add the new path to the list of final generated headers
    list(APPEND FINAL_UICd_FORMS ${FORM_NEW_PATH})
  endforeach()

  # > CMake 3.25
  # return(PROPAGATE ${output_form_headers})
  set(${output_form_headers} ${FINAL_UICd_FORMS} PARENT_SCOPE)
endfunction(QT5_WRAP_UI_SPEC)


# Make executable app.
add_executable(${APP_NAME})


# ----------- Build Modules -------------

add_subdirectory(${PROJECT_SOURCE_DIR}/src/utils)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/modules/chat)

# -------------- Build App --------------

# Add source files.
set(FORM_SRC
  ui/mainWindow.ui
  ui/configDialog.ui
)

set(QCC_QRC
  resources.qrc
)
set(MOC_HEADERS
  src/gui/animeWidget.h
  src/gui/configDialog.h
)

set(MAIN_SRC
  src/main.cpp

  icon.rc

  src/drivers/allocator.cpp
  src/drivers/coreManager.cpp
  src/drivers/eventHandler.cpp
  src/drivers/model.cpp
  src/drivers/modelManager.cpp
  src/drivers/modelParameters.cpp
  src/drivers/renderer.cpp
  src/drivers/resourceLoader.cpp
  src/drivers/textureManager.cpp
  src/drivers/wavFileHandler.cpp
  src/drivers/tools.cpp
  src/gui/animeWidget.cpp
  src/gui/configDialog.cpp
  src/gui/mainWindow.cpp
)
set(MAIN_HEADERS
  src/drivers/allocator.h
  src/drivers/coreManager.h
  src/drivers/eventHandler.h
  src/drivers/model.h
  src/drivers/modelManager.h
  src/drivers/modelParameters.h
  src/drivers/renderer.h
  src/drivers/resourceLoader.h
  src/drivers/textureManager.h
  src/drivers/wavFileHandler.h
  src/drivers/tools.h
  src/gui/mainWindow.h
)

QT5_WRAP_CPP(MOCd_HEADERS ${MOC_HEADERS})
set(GENERATED_UI_FILES_DIR ${CMAKE_BINARY_DIR}/uic)
# set UICd_FORMS
QT5_WRAP_UI_SPEC(UICd_FORMS "${FORM_SRC}" ${GENERATED_UI_FILES_DIR})
QT5_ADD_RESOURCES(RCCd_SRC ${QCC_QRC})

# Include the intermediate headers.
target_include_directories(${APP_NAME} PUBLIC ${GENERATED_UI_FILES_DIR})
target_include_directories(${APP_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src)

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
)

# Link Modules
target_link_libraries(${APP_NAME}
  utils
  modulechat
)

# Copy resource directory to build directory.
add_custom_command(
  TARGET ${APP_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E
      copy_directory ${RES_PATH} $<TARGET_FILE_DIR:${APP_NAME}>/Resources
  COMMENT "Copying resource directory ${RES_PATH} to destination"
)

# Build Framework & Core at specific platform AND link.
if(${OS} STREQUAL "windows")
  include(cmake/win.cmake)
  win_build()
else()
  include(cmake/unix.cmake)
  unix_build()
endif()
