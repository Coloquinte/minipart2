
#ifndef MINIPART_BLACKBOX_OPTIMIZER_HH
#define MINIPART_BLACKBOX_OPTIMIZER_HH

#include "hypergraph.hh"
#include "local_search.hh"

#include <random>

namespace minipart {

class BlackboxOptimizer {
 public:
  static Solution run(const Hypergraph &hypergraph, const PartitioningParams &params, const LocalSearch &localSearch);

 private:
  BlackboxOptimizer(const Hypergraph &hypergraph, const PartitioningParams &params, const LocalSearch &localSearch, std::mt19937 &rgen, std::vector<Solution> &solutions, Index level);

  Solution run();
  Solution bestSolution() const;
  void runInitialPlacement();
  void runVCycle();

  void report() const;
  void checkConsistency() const;

  static Solution computeCoarsening(const std::vector<Solution> &solutions);

 private:
  const Hypergraph &hypergraph_;
  const PartitioningParams &params_;
  const LocalSearch &localSearch_;
  std::mt19937 &rgen_;
  std::vector<Solution> &solutions_;
  Index level_;
};
} // End namespace minipart

#endif

