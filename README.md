# Minipart Hypergraph Partitioning

A hypergraph is a generalization of a graph where an edge may be connected to multiple vertices.
The hypergraph partitioning problem is to partition a hypergraph into several blocks, while keeping the number of edges that are cut to a minimum.

Minipart is a tool to partition hypergraphs. Its goal is to solve variations of the problem that arise in practice, where the objective function does not fit the textbook problem. It provides a few typical cost functions and makes it easy to plug your own.

## Building Minipart

Minipart is built using CMake. To build Minipart, make sure that you have a C++ compiler, CMake and the Boost library installed. You can then build it with the following commands:

    mkdir build; cd build
    cmake ..
    make

## Running Minipart

To run Minipart:

    ./minipart -i <input-file> -k <# of blocks> -e <% imbalance>

To see all command line options:

    ./minipart -h

To output the result:

    ./minipart -i <input-file> -k <# of blocks> -o <output-file>

To run with a different objective (here maximum block degree):

    ./minipart -i <input-file> -k <# of blocks> -g max-degree

