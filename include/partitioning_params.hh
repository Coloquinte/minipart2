
#ifndef MINIPART_PARTITIONING_PARAMS_HH
#define MINIPART_PARTITIONING_PARAMS_HH

#include "common.hh"

#include <iosfwd>

namespace minipart {

enum class ObjectiveType {
  Cut,
  Soed,
  MaxDegree
};

std::istream & operator>>(std::istream &, ObjectiveType &);
std::ostream & operator<<(std::ostream &, const ObjectiveType &);

struct PartitioningParams {
  int verbosity;
  std::size_t seed;

  // V-cycling and solution pool
  int nSolutions;
  int nCycles;

  // Coarsening options
  double minCoarseningFactor;
  double maxCoarseningFactor;
  Index minCoarseningNodes;

  // Local search options
  double movesPerElement;

  // Problem statistics
  Index nNodes;
  Index nHedges;
  Index nPins;
};

} // End namespace minipart

#endif

