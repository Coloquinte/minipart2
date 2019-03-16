
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
    double minCoarseningFactor;
    double maxCoarseningFactor;
    double movesPerElement;
    int seed;
  };

  static Solution run(const Hypergraph &hypergraph, const Params &params);

 private:
  BlackboxOptimizer(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, std::vector<Solution> &solutions);

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
};
} // End namespace minipart

#endif

