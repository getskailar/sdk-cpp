cmake_minimum_required(VERSION 3.20)

project(skailar
  VERSION 0.0.1
  DESCRIPTION "Official C++ SDK for the Skailar API"
  HOMEPAGE_URL "https://github.com/getskailar/sdk-cpp"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SKAILAR_BUILD_TESTS "Build the Skailar SDK test suite" OFF)
option(SKAILAR_BUILD_EXAMPLES "Build the Skailar SDK examples" OFF)
option(SKAILAR_INSTALL "Generate install and export rules for the Skailar SDK" ON)
option(SKAILAR_WERROR "Treat compiler warnings as errors" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

find_package(CURL REQUIRED)
find_package(nlohmann_json 3.10 CONFIG REQUIRED)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/include/skailar/version.hpp"
  @ONLY)

set(SKAILAR_SOURCES
  src/client.cpp
  src/http_client.cpp
  src/sse_parser.cpp
  src/streaming.cpp
  src/chat.cpp
  src/models.cpp
  src/images.cpp
  src/audio.cpp
  src/uploads.cpp
  src/ping.cpp
  src/errors.cpp
  src/serialization/shared_serialization.cpp
  src/serialization/chat_serialization.cpp
  src/serialization/models_serialization.cpp
  src/serialization/images_serialization.cpp
  src/serialization/audio_serialization.cpp
  src/serialization/uploads_serialization.cpp)

add_library(skailar ${SKAILAR_SOURCES})
add_library(Skailar::skailar ALIAS skailar)

set_target_properties(skailar PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)

target_include_directories(skailar
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")

# libcurl is PRIVATE: it appears only in the implementation, never in a public
# header. nlohmann_json is PUBLIC because skailar::json (an alias for
# nlohmann::json) is a value-typed field on public request/response structs, so
# consumers need its headers to use the API.
target_link_libraries(skailar PRIVATE CURL::libcurl)
target_link_libraries(skailar PUBLIC nlohmann_json::nlohmann_json)

target_compile_definitions(skailar
  PRIVATE
    $<$<BOOL:${BUILD_SHARED_LIBS}>:SKAILAR_BUILDING_SHARED>
  PUBLIC
    $<$<BOOL:${BUILD_SHARED_LIBS}>:SKAILAR_SHARED>)

if(MSVC)
  target_compile_options(skailar PRIVATE /W3 $<$<BOOL:${SKAILAR_WERROR}>:/WX>)
  target_compile_definitions(skailar PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
  target_compile_options(skailar PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<BOOL:${SKAILAR_WERROR}>:-Werror>)
endif()

if(SKAILAR_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(SKAILAR_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(SKAILAR_INSTALL)
  install(TARGETS skailar
    EXPORT skailarTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/skailar"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/include/skailar/version.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/skailar)

  install(EXPORT skailarTargets
    FILE skailarTargets.cmake
    NAMESPACE Skailar::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/skailar)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SkailarConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/skailarConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/skailar)

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/skailarConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/skailarConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/skailarConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/skailar)
endif()
