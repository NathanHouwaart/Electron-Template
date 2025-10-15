cmake_minimum_required(VERSION 3.16)

cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0042 NEW)

project(deviceaddon VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DNAPI_VERSION=9)
include_directories(${CMAKE_JS_INC} "native")

# Allow overriding or explicitly specifying the include dirs from the command line:
#  -DNODE_ADDON_API_DIR=...   (path containing napi.h / headers for node-addon-api)
set(NODE_ADDON_API_DIR "" CACHE PATH "Path to node-addon-api include directory (contains 'napi.h')")

if(NOT NODE_ADDON_API_DIR)
	# 1) Prefer a node_modules-installed node-addon-api next to the source
	if(EXISTS "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api")
		set(NODE_ADDON_API_DIR "${CMAKE_SOURCE_DIR}/node_modules/node-addon-api")
	# 2) Some packages put the headers under node-common-api
	elseif(EXISTS "${CMAKE_SOURCE_DIR}/node_modules/node-common-api")
		set(NODE_ADDON_API_DIR "${CMAKE_SOURCE_DIR}/node_modules/node-common-api")
	else()
		# 3) Ask node (if available) where node-addon-api headers live
		execute_process(
			COMMAND node -p "try{require('node-addon-api').include}catch(e){}"
			OUTPUT_VARIABLE NODE_ADDON_API_DIR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
	endif()
endif()

if(NODE_ADDON_API_DIR)
	# Normalize and add to include path
	string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
	file(TO_CMAKE_PATH ${NODE_ADDON_API_DIR} NODE_ADDON_API_DIR_PATH)
	include_directories("${NODE_ADDON_API_DIR_PATH}")
	message(STATUS "Added node-addon-api include dir: ${NODE_ADDON_API_DIR_PATH}")
else()
	message(WARNING "node-addon-api / node-common-api include dir not found. 'napi.h' may be missing.")
	message(WARNING "If headers are located under node_modules, run 'npm install' and/or set -DNODE_ADDON_API_DIR=<path>.")
endif()

# Use minimal test binding to isolate crash
set(SOURCE_FILES 
  "native/binding_minimal.cpp"
  # "native/binding.cpp"
  # "native/Device.cpp"
)

# Fix MinGW compatibility: cmake-js passes MSVC-style linker flags like /DELAYLOAD:NODE.EXE
# which break MinGW's ld. Strip them when using MinGW.
if(MINGW OR CMAKE_COMPILER_IS_GNUCXX)
  message(STATUS "Detected MinGW/GCC - stripping MSVC-style linker flags")
  # Remove /DELAYLOAD and other MSVC flags from CMAKE_SHARED_LINKER_FLAGS
  string(REGEX REPLACE "/DELAYLOAD:[^ ]+" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
  string(REGEX REPLACE "^[ \t]+" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
  message(STATUS "Cleaned linker flags: ${CMAKE_SHARED_LINKER_FLAGS}")
  
  # DO NOT STATIC LINK - causes crashes in Electron with MinGW
  # The addon will need bundled DLLs instead
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
  message(STATUS "Skipping static linking - will use dynamic DLLs to avoid Electron crashes")
endif()

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB})


