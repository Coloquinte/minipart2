
#include "local_search_optimizer.hh"
#include "move.hh"
#include "partitioning_params.hh"

using namespace std;

namespace minipart {

LocalSearchOptimizer::LocalSearchOptimizer(IncrementalObjective &inc, const PartitioningParams &params, mt19937 &rgen)
: inc_(inc)
, params_(params)
, rgen_(rgen) {
}

void LocalSearchOptimizer::run() {
  init();
  while (totalBudget() > 0) {
    doMove();
  }
}

int64_t LocalSearchOptimizer::totalBudget() const {
  int64_t ret = 0;
  for (const unique_ptr<Move> &mv : moves_) {
    if (mv->budget_ > 0) ret += mv->budget_;
  }
  return ret;
}

void LocalSearchOptimizer::init() {
  moves_.clear();
  double targetCount = params_.movesPerElement * params_.nNodes * (params_.nParts - 1);
  moves_.emplace_back(make_unique<SimpleMove>(0.1 * targetCount));
  moves_.emplace_back(make_unique<SimpleSwap>(0.1 * targetCount));
  moves_.emplace_back(make_unique<EdgeMove>(0.1 * targetCount));
  moves_.emplace_back(make_unique<AbsorptionMove>(0.7 * targetCount));
}

void LocalSearchOptimizer::doMove() {
  // Pick a move based on the remaining budget
  uniform_int_distribution<size_t> dist(0, totalBudget() - 1);
  size_t roll = dist(rgen_);
  size_t tot = 0;
  for (unique_ptr<Move> &mv : moves_) {
    if (mv->budget_ > 0) tot += mv->budget_;
    if (tot > roll) {
      mv->run(inc_, rgen_);
      return;
    }
  }
  assert (false);
}

} // End namespace minipart
