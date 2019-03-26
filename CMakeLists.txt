
cmake_minimum_required(VERSION 2.8)
PROJECT(MINIPART)

SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++14")

FIND_PACKAGE(Boost COMPONENTS system filesystem program_options unit_test_framework REQUIRED)

INCLUDE_DIRECTORIES(${MINIPART_SOURCE_DIR}/include)

SET (SOURCES
  src/hypergraph.cc
  src/solution.cc
  src/incremental_solution.cc
  src/blackbox_optimizer.cc
  src/objective_function.cc
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

