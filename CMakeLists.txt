cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

project(deviceaddon VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)



include_directories(${CMAKE_JS_INC})
include_directories(${CMAKE_SOURCE_DIR}/node_modules/node-addon-api)

add_definitions(-DNAPI_VERSION=9)
include_directories(${CMAKE_JS_INC} "addons" "build/generators/conan_includes")
file(GLOB_RECURSE SOURCE_FILES "addons/*")

# find_package(etl REQUIRED)


add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} )



add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy 
	$<TARGET_FILE:${PROJECT_NAME}> 
	${CMAKE_SOURCE_DIR}/build/${PROJECT_NAME}.node
)


