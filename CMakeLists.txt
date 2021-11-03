cmake_minimum_required(VERSION 3.18)

project(vrc_photo_album2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_COMPILER /usr/bin/g++)

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
find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(LINK_FLAGS "${LINK_FLAGS} -lboost_thread")

include_directories(${PNG_INCLUDE_DIRS})

if(NOT EXISTS ${CMAKE_BINARY_DIR}/resources)
  message(STATUS "Copy resources directory")
  file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
endif()

if(NOT EXISTS ${CMAKE_BINARY_DIR}/blank.png)
  message(STATUS "Copy blank.png")
  file(COPY ${CMAKE_SOURCE_DIR}/resources/blank.png DESTINATION ${CMAKE_BINARY_DIR})
endif()

if(NOT EXISTS ${CMAKE_BINARY_DIR}/export/png)
  message(STATUS "Create export/png directory")
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/export/png)
endif()
if(NOT EXISTS ${CMAKE_BINARY_DIR}/export/video)
  message(STATUS "Create export/video directory")
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/export/video)
endif()

add_subdirectory(src)
