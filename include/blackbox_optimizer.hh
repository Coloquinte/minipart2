
#ifndef MINIPART_BLACKBOX_OPTIMIZER_HH
#define MINIPART_BLACKBOX_OPTIMIZER_HH

#include "hypergraph.hh"
#include "partitioning_params.hh"

#include <random>

namespace minipart {
class IncrementalSolution;

class BlackboxOptimizer {
 public:
  static Solution run(const Hypergraph &hypergraph, const PartitioningParams &params);

 private:
  BlackboxOptimizer(const Hypergraph &hypergraph, const PartitioningParams &params, std::mt19937 &rgen, std::vector<Solution> &solutions, Index level);

  Solution run();
  void runInitialPlacement();
  void runVCycle();
  void runLocalSearch(Solution &solution);

  Solution bestSolution() const;
  void report() const;
  void checkConsistency() const;

  static void runMovePass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);
  static void runSwapPass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);
  static void runAbsorptionPass(IncrementalSolution &inc, Index nMoves, std::mt19937 &rgen);

  static Solution computeCoarsening(const std::vector<Solution> &solutions);

 private:
  const Hypergraph &hypergraph_;
  const PartitioningParams &params_;
  std::mt19937 &rgen_;
  std::vector<Solution> &solutions_;
  Index level_;
};
} // End namespace minipart

#endif

