##### STT Test

find_package(Qt5 COMPONENTS Test REQUIRED)

add_executable(test_stt)

set(MOC_TESTSTT_HEADERS
    ${CMAKE_SOURCE_DIR}/src/modules/stt/client.h
    ${CMAKE_SOURCE_DIR}/test/modules/test_stt.h
)
QT5_WRAP_CPP(MOCd_TESTSTT_HEADERS ${MOC_TESTSTT_HEADERS})
target_sources(test_stt
    PRIVATE
    ${MOCd_TESTSTT_HEADERS}
    ${MOC_TESTSTT_HEADERS}
    ${CMAKE_SOURCE_DIR}/src/modules/stt/client.cpp
    ${CMAKE_SOURCE_DIR}/test/modules/test_stt.cpp
)
target_link_libraries(test_stt
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Test
  utils
  moduleaudio
)
target_include_directories(test_stt
    PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdParty
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test/modules
)

##### ChatBox Manual Test

# add_executable(test_chatbox)

# target_sources(test_chatbox
#     PRIVATE
#     ${CMAKE_SOURCE_DIR}/test/gui/test_chatbox.cpp
# )
# target_link_libraries(test_chatbox
#   Qt5::Core
#   Qt5::Gui
#   Qt5::Widgets
#   utils
#   modulechat
# )
# target_include_directories(test_chatbox
#     PRIVATE
#     ${CMAKE_SOURCE_DIR}/thirdParty
#     ${CMAKE_SOURCE_DIR}/src
#     ${CMAKE_BINARY_DIR}/uic
#     ${CMAKE_SOURCE_DIR}/test/modules
# )


##### Chat Client Test

add_executable(mock_chatserver
    ${CMAKE_SOURCE_DIR}/test/modules/moc_server.cpp
)
target_include_directories(mock_chatserver
    PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdParty
)
target_link_libraries(mock_chatserver
    utils
)

add_executable(test_chatclient)

set(MOC_TESTCHATCLIENT_HEADERS
    ${CMAKE_SOURCE_DIR}/src/modules/chat/openai_client.h
    ${CMAKE_SOURCE_DIR}/test/modules/test_chatclient.h
)
QT5_WRAP_CPP(MOCd_TESTCHATCLIENT_HEADERS ${MOC_TESTCHATCLIENT_HEADERS})
target_sources(test_chatclient
    PRIVATE
    ${MOCd_TESTCHATCLIENT_HEADERS}
    ${MOC_TESTCHATCLIENT_HEADERS}
    ${CMAKE_SOURCE_DIR}/test/modules/test_chatclient.cpp
)
target_link_libraries(test_chatclient
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Test
  utils
  modulechat
)
target_include_directories(test_chatclient
    PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdParty
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/uic
)


##### Hotkey Manual Test


add_executable(test_hotkey)

set(MOC_TESTHOTKEY_HEADERS
    ${CMAKE_SOURCE_DIR}/src/modules/hotkey/shortcut_handler.h
)
QT5_WRAP_CPP(MOCd_TESTHOTKEY_HEADERS ${MOC_TESTHOTKEY_HEADERS})
target_sources(test_hotkey
    PRIVATE
    ${MOCd_TESTHOTKEY_HEADERS}
    ${MOC_TESTHOTKEY_HEADERS}
    ${CMAKE_SOURCE_DIR}/test/modules/test_hotkey.cpp
)
target_link_libraries(test_hotkey
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  utils
  modulehotkey
)
target_include_directories(test_hotkey
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)


##### MCP Config Test

add_executable(test_mcpconfig)
target_sources(test_mcpconfig
    PRIVATE
    ${CMAKE_SOURCE_DIR}/test/modules/test_mcpconfig.cpp
    ${CMAKE_SOURCE_DIR}/src/config/mcp_config.cpp
)
target_include_directories(test_mcpconfig
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thirdParty
)
target_link_libraries(test_mcpconfig
    PRIVATE
    utils
    Qt5::Core
    Qt5::Widgets
    svcore
)

##### Module Config Test

add_executable(test_moduleconfig)
target_sources(test_moduleconfig
    PRIVATE
    ${CMAKE_SOURCE_DIR}/test/modules/test_moduleconfig.cpp
    ${CMAKE_SOURCE_DIR}/src/config/module_config.cpp
)
target_include_directories(test_moduleconfig
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/thirdParty
)
target_link_libraries(test_moduleconfig
    PRIVATE
    utils
    Qt5::Core
    Qt5::Widgets
    svcore
)
