
#ifndef MINIPART_BLACKBOX_OPTIMIZER_HH
#define MINIPART_BLACKBOX_OPTIMIZER_HH

#include "hypergraph.hh"

#include <random>

namespace minipart {
class IncrementalSolution;

class BlackboxOptimizer {
 public:
  struct Params {
    int nSolutions;
    int nCycles;
    double coarseningFactor;
    double movesPerElement;
    int seed;
  };

  static Solution run(const Hypergraph &hypergraph, const Params &params);

 private:
  static void runVCycle(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, std::vector<Solution> &solutions);

  static Solution runInitialPlacement(const Hypergraph &hypergraph, const Params &params, std::mt19937&);
  static void runLocalSearch(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, Solution &solution);
  static void runMovePass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);
  static void runSwapPass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);

  static Solution computeCoarsening(const std::vector<Solution> &solutions);
};
} // End namespace minipart

#endif

