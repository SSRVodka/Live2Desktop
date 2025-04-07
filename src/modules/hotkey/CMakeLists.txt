
set(HOTKEY_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/shortcut_handler.cpp
)
set(HOTKEY_H
)
set(HOTKEY_MOC_H
    ${CMAKE_CURRENT_SOURCE_DIR}/shortcut_handler.h
)

# platform-dependent headers
if (${OS} STREQUAL "linux")
    list(APPEND HOTKEY_MOC_H ${CMAKE_CURRENT_SOURCE_DIR}/platform/hotkey_linux.h)
elseif(${OS} STREQUAL "windows")
    list(APPEND HOTKEY_MOC_H ${CMAKE_CURRENT_SOURCE_DIR}/platform/hotkey_win.h)
else()
    message(FATAL "module hotkey: ${OS} is currently not supported")
endif()

QT5_WRAP_CPP(HOTKEY_MOCd ${HOTKEY_MOC_H})

if(BUILD_SHARED_LIBS)
    add_library(modulehotkey
        SHARED
        ${HOTKEY_SRC}
        ${HOTKEY_H}
        ${HOTKEY_MOCd}
        ${HOTKEY_MOC_H}
    )
else()
    add_library(modulehotkey
        STATIC
        ${HOTKEY_SRC}
        ${HOTKEY_H}
        ${HOTKEY_MOCd}
        ${HOTKEY_MOC_H}
    )
endif(BUILD_SHARED_LIBS)

# Include headers for current module
target_include_directories(modulehotkey
    PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(modulehotkey
    Qt5::Core
)
target_link_libraries(modulehotkey
    utils
)

# platform libraries
if (${OS} STREQUAL "linux")
    target_link_libraries(modulehotkey
        X11
    )
elseif(${OS} STREQUAL "windows")
    target_link_libraries(modulehotkey
        # libraries?
    )
else()
    message(FATAL "module hotkey: ${OS} is currently not supported")
endif()