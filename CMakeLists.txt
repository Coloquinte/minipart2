
cmake_minimum_required(VERSION 2.8)
PROJECT(MINIPART)

SET(CMAKE_CXX_FLAGS "-g -Wall -std=c++14")

#SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
#SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost REQUIRED COMPONENTS system filesystem iostreams program_options unit_test_framework)
FIND_PACKAGE(ZLIB REQUIRED)

INCLUDE_DIRECTORIES(
    ${MINIPART_SOURCE_DIR}/include
    ${MINIPART_BINARY_DIR}
)

SET(MINIPART_VERSION_NUMBER \"0.1.1\")
STRING(TIMESTAMP MINIPART_BUILD_DATE \"%Y-%m-%d\")

configure_file (
  "${MINIPART_SOURCE_DIR}/include/config.hh.in"
  "${MINIPART_BINARY_DIR}/config.hh"
)

SET(SOURCES
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
target_link_libraries(libminipart
  ${ZLIB_LIBRARIES}
  ${Boost_LIBRARIES})

SET(BIN_SOURCES
  src/main.cc)

add_executable(minipart ${BIN_SOURCES})
target_link_libraries(minipart
  ${ZLIB_LIBRARIES}
  ${Boost_LIBRARIES}
  libminipart
)

