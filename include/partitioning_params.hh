
#ifndef MINIPART_PARTITIONING_PARAMS_HH
#define MINIPART_PARTITIONING_PARAMS_HH

#include "common.hh"

namespace minipart {

struct PartitioningParams {
  int verbosity;
  std::size_t seed;

  // V-cycling and solution pool
  int nSolutions;
  int nCycles;

  // Coarsening options
  double minCoarseningFactor;
  double maxCoarseningFactor;

  // Local search options
  double movesPerElement;

  // Problem statistics
  Index nNodes;
  Index nHedges;
  Index nPins;
};

} // End namespace minipart

#endif

