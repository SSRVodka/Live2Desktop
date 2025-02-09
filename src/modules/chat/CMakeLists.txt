
# Project Module

find_package(Qt5 COMPONENTS Network REQUIRED)

set(CHAT_FORM
    ${PROJECT_SOURCE_DIR}/ui/chatBox.ui
    ${PROJECT_SOURCE_DIR}/ui/popup.ui
)

set(CHAT_MOC_H
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/chatBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/client.h
    ${CMAKE_CURRENT_SOURCE_DIR}/server.h
)

set(CHAT_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/chatBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/popup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)
set(CHAT_H
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/popup.h
)

QT5_WRAP_CPP(CHAT_MOCd ${CHAT_MOC_H})
set(GENERATED_UI_FILES_DIR ${CMAKE_BINARY_DIR}/uic)
QT5_WRAP_UI_SPEC(CHAT_UICd "${CHAT_FORM}" ${GENERATED_UI_FILES_DIR})

if(USE_SHARED_LIB)
    add_library(modulechat
        SHARED
        ${CHAT_MOCd}
        ${CHAT_UICd}
        ${CHAT_SRC}
        ${CHAT_H}
    )
    add_custom_command(
        TARGET modulechat
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:modulechat> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying module[chat] dynamic library to destination"
    )
else()
    add_library(modulechat
        STATIC
        ${CHAT_MOCd}
        ${CHAT_UICd}
        ${CHAT_SRC}
        ${CHAT_H}
    )
endif(USE_SHARED_LIB)

target_include_directories(modulechat
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${GENERATED_UI_FILES_DIR}
)

target_link_libraries(modulechat
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)
target_link_libraries(modulechat
    utils
)