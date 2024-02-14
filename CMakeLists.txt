cmake_minimum_required(VERSION 3.16)

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
# Set configuration (Release and Debug only).
set(CMAKE_CONFIGURATION_TYPES Debug Release
  CACHE STRING "Configurations" FORCE
)

# Debug for default.
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")

# Specify version of compiler.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Specify Cubism Framework rendering.
set(FRAMEWORK_SOURCE OpenGL)

# Find Qt libraries
find_package(Qt5 COMPONENTS Core Gui Widgets OpenGL REQUIRED)

# ----------------------------------------------------------------------------------------

# Add Cubism Core.
# Import as static library.
add_library(Live2DCubismCore STATIC IMPORTED)
# Find library path.
set_target_properties(Live2DCubismCore
  PROPERTIES
    IMPORTED_LOCATION ${CORE_PATH}/lib/libLive2DCubismCore.a
    INTERFACE_INCLUDE_DIRECTORIES ${CORE_PATH}/include
)


# Add Cubism Native Framework.
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Framework)
# Add rendering definition to framework.
target_compile_definitions(Framework PUBLIC CSM_TARGET_LINUX_GL)

# Link libraries to framework.
target_link_libraries(Framework Live2DCubismCore)

# ----------------------------------------------------------------------------


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

# Link libraries to app.
target_link_libraries(${APP_NAME}
  Framework
  ${CMAKE_DL_LIBS}
  X11
  pthread
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

# Copy resource directory to build directory.
add_custom_command(
  TARGET ${APP_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E
      copy_directory ${RES_PATH} $<TARGET_FILE_DIR:${APP_NAME}>/Resources
)

# -----------------------------


# You can change target that renderer draws by enabling following definition.
#
# * USE_RENDER_TARGET
#   Renderer draws to target of LAppView.
# * USE_MODEL_RENDER_TARGET
#   Renderer draws to target of each Model.
# * default
#   Renderer draws to default main framebuffer.
#
# INFO: USE_RENDER_TARGET has higher priority than USE_MODEL_RENDER_TARGET.
#
# target_compile_definitions(${APP_NAME}
#   PRIVATE
#     USE_RENDER_TARGET
#     USE_MODEL_RENDER_TARGET
# )
