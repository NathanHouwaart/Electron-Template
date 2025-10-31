cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(test_desfireKey test_desfireKey.cpp)

target_include_directories(test_desfireKey PRIVATE
  ${CMAKE_SOURCE_DIR}/addons/PN532_Controller/Source
  ${CMAKE_SOURCE_DIR}/addons/PN532_Controller/Headers
)

target_link_libraries(test_desfireKey
  PRIVATE
    GTest::gtest_main
    libcppdes
)

include(GoogleTest)
gtest_discover_tests(test_desfireKey)