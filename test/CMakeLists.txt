
add_executable(test_stt)

QT5_WRAP_CPP(MOCd_TESTSTT_HEADERS 
    ${CMAKE_SOURCE_DIR}/src/modules/stt/client.h
    ${CMAKE_SOURCE_DIR}/test/modules/test_stt.h
)
target_sources(test_stt
    PRIVATE
    ${MOCd_TESTSTT_HEADERS}
    ${CMAKE_SOURCE_DIR}/src/modules/stt/server.h
    ${CMAKE_SOURCE_DIR}/src/modules/stt/client.h
    ${CMAKE_SOURCE_DIR}/src/modules/stt/client.cpp
    ${CMAKE_SOURCE_DIR}/test/modules/test_stt.h
    ${CMAKE_SOURCE_DIR}/test/modules/test_stt.cpp
)
target_link_libraries(test_stt
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  utils
)
target_include_directories(test_stt
    PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdParty
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test/modules
)