
cmake_minimum_required(VERSION 2.8)
PROJECT(MINIPART)

SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++14")

FIND_PACKAGE(Boost COMPONENTS system filesystem program_options unit_test_framework REQUIRED)

INCLUDE_DIRECTORIES(
    ${MINIPART_SOURCE_DIR}/include
    ${MINIPART_BINARY_DIR}
)

set (MINIPART_VERSION_MAJOR 0)
set (MINIPART_VERSION_MINOR 1)
set (MINIPART_VERSION_PATCH 0)
set (MINIPART_BUILD_DATE string(TIMESTAMP TODAY "%Y-%m-%d"))

configure_file (
  "${MINIPART_SOURCE_DIR}/include/config.hh.in"
  "${MINIPART_BINARY_DIR}/config.hh"
)

SET (SOURCES
  src/hypergraph.cc
  src/solution.cc
  src/partitioning_params.cc
  src/incremental_objective.cc
  src/objective.cc
  src/move.cc
  src/local_search_optimizer.cc
  src/blackbox_optimizer.cc
  src/io.cc
)

add_library(libminipart ${SOURCES})

SET (BIN_SOURCES
  src/main.cc)

add_executable(minipart ${BIN_SOURCES})
target_link_libraries(minipart
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  libminipart
)

