set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

file(GLOB_RECURSE SOURCE_FILES ./*.cc)

add_executable(vrc_photo_album2 ${SOURCE_FILES})
target_link_libraries(
  vrc_photo_album2
  ${GMXX_LIBS}
  ${OpenCV_LIBS}
  Threads::Threads)
