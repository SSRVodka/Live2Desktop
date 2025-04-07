
find_package(Qt5 COMPONENTS Network REQUIRED)

set(TTS_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
)
set(TTS_H
)
set(TTS_MOC_H
    ${CMAKE_CURRENT_SOURCE_DIR}/client.h
)

QT5_WRAP_CPP(TTS_MOCd ${TTS_MOC_H})

if(BUILD_SHARED_LIBS)
    add_library(moduletts
        SHARED
        ${TTS_SRC}
        ${TTS_H}
        ${TTS_MOCd}
    )
else()
    add_library(moduletts
        STATIC
        ${TTS_SRC}
        ${TTS_H}
        ${TTS_MOCd}
    )
endif(BUILD_SHARED_LIBS)

# Include headers for current module
target_include_directories(moduletts
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/thirdParty
)

target_link_libraries(moduletts
    Qt5::Core
    Qt5::Network
)
target_link_libraries(moduletts
    utils
    svcore
)