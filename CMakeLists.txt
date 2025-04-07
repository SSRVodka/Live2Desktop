cmake_minimum_required(VERSION 3.16)

# -------------- Prebuilt-configurations --------------

# Set app name.
set(APP_NAME Live2Desktop)
# Set directory paths.
set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CORE_PATH ${ROOT_PATH}/Core)
set(FRAMEWORK_PATH ${ROOT_PATH}/Framework)
set(THIRD_PARTY_PATH ${ROOT_PATH}/thirdParty)
set(RES_PATH ${ROOT_PATH}/Resources)

# Set project.
project(${APP_NAME} VERSION 0.2)

# Define output directory
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE STRING "")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib CACHE STRING "")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin CACHE STRING "")

# Specify version of compiler.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "i386")
  set(ARCH "x86" CACHE STRING "" FORCE)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
  set(ARCH "x86_64" CACHE STRING "" FORCE)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "AMD64")
  set(ARCH "x86_64" CACHE STRING "" FORCE)
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(ARCH "arm64" CACHE STRING "" FORCE)
else()
  message(WARNING "Untested architecture detected (${CMAKE_HOST_SYSTEM_PROCESSOR}). Use x86_64 libraries instead.")
  set(ARCH "x86_64" CACHE STRING "" FORCE)
endif()

message(STATUS "Target architecture: ${ARCH}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(OS "linux" CACHE STRING "" FORCE)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(OS "windows" CACHE STRING "" FORCE)
else()
  set(OS "macos" CACHE STRING "" FORCE)
endif()

message(STATUS "Target OS: ${OS}")

if(DEBUG)
 set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
else()
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

message(STATUS "Target build type: ${CMAKE_BUILD_TYPE}")

# Use shared libraries or static libraries
# default: shared
option(BUILD_SHARED_LIBS "Live2Desktop Project Build Type" ON)

if(BUILD_SHARED_LIBS)
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
add_subdirectory(${PROJECT_SOURCE_DIR}/src/modules/audio)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/modules/hotkey)
# dependencies for module[stt]
# USE_QT is an option (default OFF) in sv.cpp
set(USE_QT ON CACHE BOOL "Enable Qt support in thirdParty/sv.cpp")
add_subdirectory(${PROJECT_SOURCE_DIR}/thirdParty/sv.cpp)
add_subdirectory(${PROJECT_SOURCE_DIR}/src/modules/stt)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/modules/tts)

add_subdirectory(${PROJECT_SOURCE_DIR}/src/modules/chat)

# -------------- Build App --------------

# Add source files.
set(FORM_SRC
  ui/mainWindow.ui
  ui/configDialog.ui
  ui/chatBox.ui
  ui/popup.ui
)

set(QCC_QRC
  resources.qrc
)
set(MOC_HEADERS
  src/gui/animeWidget.h
  src/gui/configDialog.h
  src/gui/mainWindow.h
  src/gui/chatBox.h
  src/gui/messagebubble.h
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
  src/gui/chatBox.cpp
  src/gui/messagebubble.cpp
  src/gui/popup.cpp
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

  src/gui/popup.h
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

# Include third party libraries for all the projects (including subprojects)
include_directories(${THIRD_PARTY_PATH})

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
  moduleaudio
  modulehotkey
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

# --------- Modules Config Manager ----------

# Add config manager & config tasks
target_sources(${APP_NAME}
  PRIVATE
  src/config/module_config.cpp
  src/config/module_config.h
  src/config/mcp_config.cpp
  src/config/mcp_config.h
)

# move configurations to destination
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)

add_custom_command(
    TARGET ${APP_NAME}
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E
            copy ${PROJECT_SOURCE_DIR}/config/mcp_config.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/mcp_config.json
    COMMENT "Copying configuration template mcp_config.json to destination"
)
add_custom_command(
    TARGET ${APP_NAME}
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E
            copy ${PROJECT_SOURCE_DIR}/config/module_config.json ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config/module_config.json
    COMMENT "Copying configuration template module_config.json to destination"
)


# -------------- Build Tests --------------

add_subdirectory(${PROJECT_SOURCE_DIR}/test)
