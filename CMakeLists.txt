cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

project(deviceaddon VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)

# # Add ETL as a static library
add_subdirectory(${CMAKE_SOURCE_DIR}/addons/external/etl)

# Add Tiny AES as a static library
add_subdirectory(${CMAKE_SOURCE_DIR}/addons/external/tiny-aes)

# add CppDes as a static library
add_subdirectory(${CMAKE_SOURCE_DIR}/addons/external/cppDes)

include_directories(${CMAKE_JS_INC})
include_directories(${CMAKE_SOURCE_DIR}/node_modules/node-addon-api)

add_definitions(-DNAPI_VERSION=9)
include_directories(${CMAKE_JS_INC} "addons" "build/generators/conan_includes" ${CMAKE_SOURCE_DIR}/addons/external/des/include ${CMAKE_SOURCE_DIR}/addons/external/cppDes/include)

file(GLOB_RECURSE ALL_ADDON_FILES
    "${CMAKE_SOURCE_DIR}/addons/*.c"
    "${CMAKE_SOURCE_DIR}/addons/*.cpp"
    "${CMAKE_SOURCE_DIR}/addons/*.cc"
    "${CMAKE_SOURCE_DIR}/addons/*.cxx"
    "${CMAKE_SOURCE_DIR}/addons/*.h"
    "${CMAKE_SOURCE_DIR}/addons/*.hpp"
    "${CMAKE_SOURCE_DIR}/addons/*.hh"
)

# Filter out external libraries, tests and example programs that may define their own main()
set(SOURCE_FILES)
foreach(_f ${ALL_ADDON_FILES})
    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_f}")
    string(REPLACE "\\" "/" _rel_norm "${_rel}")

    if(_rel_norm MATCHES "^addons/external/.*")
        continue()
    endif()

    get_filename_component(_name "${_f}" NAME)
    if(_name MATCHES "^test\\.(c|cpp)$")
        continue()
    endif()

    list(APPEND SOURCE_FILES ${_f})
endforeach()

# Add DES static library
add_library(des STATIC ${CMAKE_SOURCE_DIR}/addons/external/des/des.c)
target_include_directories(des PUBLIC
    ${CMAKE_SOURCE_DIR}/addons/external/des/
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_JS_INC}
    ${CMAKE_SOURCE_DIR}/node_modules/node-addon-api
    ${CMAKE_SOURCE_DIR}/addons
    ${CMAKE_SOURCE_DIR}/build/generators/conan_includes
    ${CMAKE_SOURCE_DIR}/addons/external/mbedtls/include
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MBEDTLS_CONFIG_FILE="mbedtls_config_desfire.h"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_JS_LIB}
        tiny-aes
        des
        libcppdes
        etl::etl
)


# Enable CTest at the top-level so tests added in subdirectories are
# registered in the top-level CTestTestfile.cmake and discovered by ctest.
enable_testing()

add_subdirectory(${CMAKE_SOURCE_DIR}/addons/PN532_Controller/tests)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}.node
)
