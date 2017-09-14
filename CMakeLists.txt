# @TODO: move to separate cmake
macro(use_cxx11)
  if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      set (CMAKE_CXX_FLAGS "-std=gnu++11 ${CMAKE_CXX_FLAGS}")
    endif ()
  else ()
    set (CMAKE_CXX_STANDARD 11)
  endif ()
endmacro(use_cxx11)

cmake_minimum_required(VERSION 2.8)

use_cxx11()

project(secret-passage)

find_package(
  Boost
  COMPONENTS
    system
    program_options
    thread
    log
)

file(GLOB SRC
    "*.hpp"
    "*.cpp"
)

add_executable(
  ${PROJECT_NAME}
    ${Boost_INCLUDE_DIR}
    ${SRC}
)

target_link_libraries(
  ${PROJECT_NAME}
    ${Boost_LIBRARIES}
)
