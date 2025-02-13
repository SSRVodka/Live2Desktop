
# Project Module


# FFMPEG for submodule whisper
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(WHISPER_FFMPEG "whisper: support building and linking with ffmpeg libs (avcodec, swresample, ...)" OFF)
endif()


set(STT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)
set(STT_H
    ${CMAKE_CURRENT_SOURCE_DIR}/server.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/common.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/dr_wav.h
    ${CMAKE_CURRENT_SOURCE_DIR}/common/json.hpp
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
    add_custom_command(
        TARGET modulestt
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:modulestt> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying module[stt] dynamic library to destination"
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
