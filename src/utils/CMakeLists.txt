
# Project Common

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} UTILS_SRC)

if(USE_SHARED_LIB)
    add_library(utils
        SHARED
        ${UTILS_SRC}
    )
    add_custom_command(
        TARGET utils
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E
            copy $<TARGET_FILE:utils> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        COMMENT "Copying utils dynamic library to destination"
    )
else()
    add_library(utils
        STATIC
        ${UTILS_SRC}
    )
endif(USE_SHARED_LIB)

target_include_directories(utils
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
