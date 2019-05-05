
#ifndef MINIPART_LOCAL_SEARCH_OPTIMIZER_HH
#define MINIPART_LOCAL_SEARCH_OPTIMIZER_HH

#include "hypergraph.hh"
#include "incremental_objective.hh"
#include "move.hh"

#include <random>
#include <iosfwd>
#include <cassert>
#include <memory>

namespace minipart {

class Move;

class LocalSearchOptimizer {
 public:
  LocalSearchOptimizer(IncrementalObjective &inc, const PartitioningParams &params, std::mt19937 &rgen);
  void run();

 private:
  void init();
  void doMove();
  std::int64_t totalBudget() const;

 private:
  IncrementalObjective &inc_;
  const PartitioningParams &params_;
  std::mt19937 &rgen_;
  std::vector<std::unique_ptr<Move> > moves_;
};

} // End namespace minipart

#endif

