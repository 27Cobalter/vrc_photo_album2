cmake_minimum_required(VERSION 3.18)

project(vrc_photo_album2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -O2 -pipe -fstack-protector-strong -fno-plt -fno-omit-frame-pointer"
)

if(CMAKE_GENEROTOR STREQUAL "Ninja")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()

message(STATUS "Detecting CPU architecture " ${CMAKE_SYSTEM_PROCESSOR})
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a+crc")
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
include_directories(${PROJECT_SOURCE_DIR}/submodule)

execute_process(
  COMMAND Magick++-config --cppflags
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GMXX_CPP_FLAGS)
execute_process(
  COMMAND Magick++-config --cxxflags
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GMXX_CXX_FLAGS)
execute_process(
  COMMAND Magick++-config --ldflags
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GMXX_LD_FLAGS)
execute_process(
  COMMAND Magick++-config --libs
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE GMXX_LIBS)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GMXX_CPP_FLAGS} ${GMXX_CXX_FLAGS}")
set(LINK_FLAGS "${LINK_FLAGS} ${GMXX_LD_FLAGS}")

set(LINK_FLAGS "${LINK_FLAGS} -lboost_thread")

if(NOT EXISTS ${CMAKE_BINARY_DIR}/resources)
  message(STATUS "Copy resources directory")
  file(COPY ${CMAKE_SOURCE_DIR}/submodule/vrc_photo_streamer/resources DESTINATION ${CMAKE_BINARY_DIR})
endif()

add_subdirectory(src)
