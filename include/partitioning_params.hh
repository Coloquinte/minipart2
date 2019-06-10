
#ifndef MINIPART_PARTITIONING_PARAMS_HH
#define MINIPART_PARTITIONING_PARAMS_HH

#include "common.hh"

#include <iosfwd>

namespace minipart {

enum class ObjectiveType {
  /*
   * Minimize the number of edges cut
   under capacity constraints
   */
  Cut,

  /*
   * Minimize the sum of the edge degrees (connectivity)
   * under capacity constraints
   */
  Soed,

  /*
   * Minimize the degree of the most connected block
   * under capacity constraints
   */
  MaxDegree,

  /*
   * Minimize the sum of distances
   * in a daisy chain topology
   * under capacity constraints
   */
  DaisyChainDistance,

  /*
   * Minimize the degree of the most connected block
   * in a daisy chain topology
   * under capacity constraints
   */
  DaisyChainMaxDegree,

  /*
   * Minimize the number of edges cut
   * scaled by block usage
   */
  RatioCut,

  /*
   * Minimize the sum of the edge degrees (connectivity),
   * scaled by block usage
   */
  RatioSoed,

  /*
   * Minimize the degree of the most connected block
   * scaled by block usage
   */
  RatioMaxDegree
};

std::istream & operator>>(std::istream &, ObjectiveType &);
std::ostream & operator<<(std::ostream &, const ObjectiveType &);

struct PartitioningParams {
  int verbosity;
  std::size_t seed;
  ObjectiveType objective;

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
  Index nParts;
};

} // End namespace minipart

#endif

