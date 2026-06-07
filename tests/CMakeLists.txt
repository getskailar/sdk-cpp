find_package(GTest CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)

add_executable(skailar_tests
  test_client.cpp
  test_chat.cpp
  test_streaming.cpp
  test_sse_parser.cpp
  test_errors.cpp
  test_retries.cpp
  test_audio.cpp
  test_images.cpp
  test_uploads.cpp
  test_models.cpp)

target_link_libraries(skailar_tests
  PRIVATE
    Skailar::skailar
    GTest::gtest
    GTest::gtest_main
    httplib::httplib)

# Tests reach into src/ for the internal SSE parser and string utilities.
target_include_directories(skailar_tests PRIVATE "${CMAKE_SOURCE_DIR}/src")

if(MSVC)
  target_compile_options(skailar_tests PRIVATE /W3)
else()
  target_compile_options(skailar_tests PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(skailar_tests)
