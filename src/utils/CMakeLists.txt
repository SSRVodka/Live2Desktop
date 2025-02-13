
# Project Common

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} UTILS_SRC)

if(BUILD_SHARED_LIBS)
    add_library(utils
        SHARED
        ${UTILS_SRC}
    )
else()
    add_library(utils
        STATIC
        ${UTILS_SRC}
    )
endif(BUILD_SHARED_LIBS)

target_include_directories(utils
    PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
