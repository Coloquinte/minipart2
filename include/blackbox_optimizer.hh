
#ifndef MINIPART_BLACKBOX_OPTIMIZER_HH
#define MINIPART_BLACKBOX_OPTIMIZER_HH

#include "hypergraph.hh"

#include <random>

namespace minipart {
class BlackboxOptimizer {
 public:
  struct Params {
    int nCycles;
    double coarseningFactor;
    double movesPerElement;
    int seed;
  };

  static std::vector<Index> run(const Hypergraph &hypergraph, const Params &params);

 private:
  static std::vector<Index> runInitialPlacement(const Hypergraph &hypergraph, const Params &params, std::mt19937&);
  static void runLocalSearch(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, std::vector<Index> &solution);
  static void runVCycle(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, std::vector<std::vector<Index> > &solutions);

  static std::vector<Index> computeCoarsening(const std::vector<std::vector<Index> > &solutions);
};
} // End namespace minipart

#endif

