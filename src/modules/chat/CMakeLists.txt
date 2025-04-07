
# Project Module

find_package(Qt5 COMPONENTS Network REQUIRED)


set(CHAT_MOC_H
    ${CMAKE_CURRENT_SOURCE_DIR}/openai_client.h
)

set(CHAT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/openai_client.cpp
)
set(CHAT_H
)

QT5_WRAP_CPP(CHAT_MOCd ${CHAT_MOC_H})

if(BUILD_SHARED_LIBS)
    add_library(modulechat
        SHARED
        ${CHAT_MOCd}
        ${CHAT_UICd}
        ${CHAT_SRC}
        ${CHAT_H}
    )
else()
    add_library(modulechat
        STATIC
        ${CHAT_MOCd}
        ${CHAT_UICd}
        ${CHAT_SRC}
        ${CHAT_H}
    )
endif(BUILD_SHARED_LIBS)

# Include headers for current module
target_include_directories(modulechat
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/thirdParty
    ${GENERATED_UI_FILES_DIR}
)

target_link_libraries(modulechat
    Qt5::Core
    Qt5::Network
)
target_link_libraries(modulechat
    utils
)