
#ifndef MINIPART_BLACKBOX_OPTIMIZER_HH
#define MINIPART_BLACKBOX_OPTIMIZER_HH

#include "hypergraph.hh"
#include "local_search.hh"

#include <random>
#include <string>

namespace minipart {

class BlackboxOptimizer {
 public:
  static Solution run(const Hypergraph &hypergraph, const PartitioningParams &params, const Objective &objective);

 private:
  BlackboxOptimizer(const Hypergraph &hypergraph, const PartitioningParams &params, const Objective &objective, std::mt19937 &rgen, std::vector<Solution> &solutions, Index level);

  Solution run();
  Solution bestSolution() const;
  void runInitialPlacement();
  void runLocalSearch();
  void runVCycle();

  void report(const std::string &step) const;
  void report(const std::string &step, Index nSols) const;
  void reportStartCycle() const;
  void reportEndCycle() const;
  void reportStartSearch() const;
  void reportEndSearch() const;

  void checkConsistency() const;

  static Solution computeCoarsening(const std::vector<Solution> &solutions);

 private:
  const Hypergraph &hypergraph_;
  const PartitioningParams &params_;
  const Objective &objective_;
  std::mt19937 &rgen_;
  std::vector<Solution> &solutions_;
  Index level_;
  Index cycle_;
};
} // End namespace minipart

#endif

