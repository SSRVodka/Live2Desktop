
# Project Module


# FFMPEG for submodule whisper
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(WHISPER_FFMPEG "whisper: support building and linking with ffmpeg libs (avcodec, swresample, ...)" OFF)
endif()


set(STT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
)
set(STT_H
)
set(STT_MOC_H
    ${CMAKE_CURRENT_SOURCE_DIR}/client.h
)

QT5_WRAP_CPP(STT_MOCd ${STT_MOC_H})

if(BUILD_SHARED_LIBS)
    add_library(modulestt
        SHARED
        ${STT_SRC}
        ${STT_H}
        ${STT_MOCd}
    )
else()
    add_library(modulestt
        STATIC
        ${STT_SRC}
        ${STT_H}
        ${STT_MOCd}
    )
endif(BUILD_SHARED_LIBS)

# Include headers for current module
target_include_directories(modulestt
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/thirdParty
)

target_link_libraries(modulestt
    Qt5::Core
)
target_link_libraries(modulestt
    utils
    svcore
)


# Cli demo
add_executable(stt_server
    ${CMAKE_CURRENT_SOURCE_DIR}/server_main.cpp
)
target_include_directories(stt_server
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/thirdParty
)
target_link_libraries(stt_server
    modulestt
)
