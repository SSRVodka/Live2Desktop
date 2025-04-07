
# Project Module

find_package(Qt5 COMPONENTS Multimedia REQUIRED)

# set(MODULE_AUDIO_FORM
# )

set(MODULE_AUDIO_MOC_H
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_recorder.h
)

set(MODULE_AUDIO_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/audio_recorder.cpp
)
set(MODULE_AUDIO_H
)

QT5_WRAP_CPP(MODULE_AUDIO_MOCd ${MODULE_AUDIO_MOC_H})
# set(GENERATED_UI_FILES_DIR ${CMAKE_BINARY_DIR}/uic)
# QT5_WRAP_UI_SPEC(MODULE_AUDIO_UICd "${MODULE_AUDIO_FORM}" ${GENERATED_UI_FILES_DIR})

if(BUILD_SHARED_LIBS)
    add_library(moduleaudio
        SHARED
        ${MODULE_AUDIO_MOCd}
        ${MODULE_AUDIO_UICd}
        ${MODULE_AUDIO_SRC}
        ${MODULE_AUDIO_H}
    )
else()
    add_library(moduleaudio
        STATIC
        ${MODULE_AUDIO_MOCd}
        ${MODULE_AUDIO_UICd}
        ${MODULE_AUDIO_SRC}
        ${MODULE_AUDIO_H}
    )
endif(BUILD_SHARED_LIBS)

# Include headers for current module
target_include_directories(moduleaudio
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/thirdParty
    ${GENERATED_UI_FILES_DIR}
)

target_link_libraries(moduleaudio
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
)
target_link_libraries(moduleaudio
    utils
    modulestt
    moduletts
)