
#ifndef MINIPART_BLACKBOX_OPTIMIZER_HH
#define MINIPART_BLACKBOX_OPTIMIZER_HH

#include "hypergraph.hh"

#include <random>

namespace minipart {
class IncrementalSolution;

class BlackboxOptimizer {
 public:
  struct Params {
    int seed;
    // V-cycling and solution pool
    int nSolutions;
    int nCycles;
    // Coarsening options
    double minCoarseningFactor;
    double maxCoarseningFactor;
    // Local search options
    double movesPerElement;
  };

  static Solution run(const Hypergraph &hypergraph, const Params &params);

 private:
  BlackboxOptimizer(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, std::vector<Solution> &solutions, Index level);

  void runVCycle();
  void runLocalSearch(Solution &solution);

  void report() const;
  void checkConsistency() const;

  static Solution runInitialPlacement(const Hypergraph &hypergraph, const Params &params, std::mt19937&);
  static void runMovePass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);
  static void runSwapPass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);

  static Solution computeCoarsening(const std::vector<Solution> &solutions);

 private:
  const Hypergraph &hypergraph_;
  const Params &params_;
  std::mt19937 &rgen_;
  std::vector<Solution> &solutions_;
  Index level_;
};
} // End namespace minipart

#endif

