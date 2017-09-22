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

set(Boost_USE_STATIC_LIBS ON)
find_package(
  Boost
  COMPONENTS
    log
    log_setup
    system
    filesystem
    program_options
    thread
)

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL)

message("OPENSSL libs: ${OPENSSL_LIBRARIES}")

file(GLOB SRC
    "*.hpp"
    "*.cpp"
    "http-parser/http_parser.h" "http-parser/http_parser.c"
)

add_definitions(
  -DBOOST_LOG_USE_NATIVE_SYSLOG
)

add_executable(
  ${PROJECT_NAME}
    ${Boost_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${SRC}
)

target_link_libraries(
  ${PROJECT_NAME}
    ${Boost_LIBRARIES}
#    ${OPENSSL_LIBRARIES}
    ${CMAKE_DL_LIBS}
    libssl.a  # for some reason OPENSSL_USE_STATIC_LIBS does not work
    libcrypto.a
)
