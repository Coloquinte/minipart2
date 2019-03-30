// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "blackbox_optimizer.hh"
#include "objective_function.hh"
#include "partitioning_params.hh"

#include <iostream>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <limits>

using namespace std;

namespace minipart {
BlackboxOptimizer::BlackboxOptimizer(const Hypergraph &hypergraph, const PartitioningParams &params, const LocalSearch &localSearch, std::mt19937 &rgen, std::vector<Solution> &solutions, Index level)
: hypergraph_(hypergraph)
, params_(params)
, localSearch_(localSearch)
, rgen_(rgen)
, solutions_(solutions)
, level_(level) {
}

void BlackboxOptimizer::runInitialPlacement() {
  solutions_.clear();
  for (Index i = 0; i < params_.nSolutions; ++i) {
    uniform_int_distribution<int> partDist(0, hypergraph_.nParts()-1);
    Solution solution(hypergraph_.nNodes(), hypergraph_.nParts());
    for (Index i = 0; i < hypergraph_.nNodes(); ++i) {
      solution[i] = partDist(rgen_);
    }
    solutions_.push_back(solution);
  }
}

void BlackboxOptimizer::report() const {
  if (solutions_.empty()) {
    cout << "No solution in the pool" << endl;
    return;
  }
  Solution solution = bestSolution();
  localSearch_.report(hypergraph_, solution, cout);
  cout << endl;
}

Solution BlackboxOptimizer::run(const Hypergraph &hypergraph, const PartitioningParams &params, const LocalSearch &localSearch) {
  mt19937 rgen(params.seed);
  vector<Solution> sols;
  BlackboxOptimizer opt(hypergraph, params, localSearch, rgen, sols, 0);
  return opt.run();
}

Solution BlackboxOptimizer::run() {
  runInitialPlacement();
  for (int i = 0; i < params_.nCycles; ++i) {
    if (params_.verbosity >= 2)
      cout << "Starting V-cycle #" << i + 1 << endl;
    runVCycle();
    if (params_.verbosity >= 2)
      report();
  }
  if (params_.verbosity >= 2)
    cout << endl;

  return bestSolution();
}

Solution BlackboxOptimizer::bestSolution() const {
  assert (!solutions_.empty());
  size_t best = 0;
  for (size_t i = 1; i < solutions_.size(); ++i) {
    if (localSearch_.compare(hypergraph_, solutions_[i], solutions_[best])) {
      best = i;
    }
  }
  return solutions_[best];
}

namespace {
class SolutionHasher {
 public:
  SolutionHasher(const vector<Solution> &solutions)
    : solutions_(solutions) {}

  uint64_t operator()(const Index &node) const {
    // FNV hash
    uint64_t magic = 1099511628211llu;
    uint64_t ret = 0;
    for (const Solution &solution : solutions_) {
      ret = (ret ^ (uint64_t)solution[node]) * magic;
    }
    return ret;
  }

 private:
  const vector<Solution> &solutions_;
};

class SolutionComparer {
 public:
  SolutionComparer(const vector<Solution> &solutions)
    : solutions_(solutions) {}

  bool operator()(const Index &n1, const Index &n2) const {
    for (const Solution &solution : solutions_) {
      if (solution[n1] != solution[n2]) return false;
    }
    return true;
  }

 private:
  const vector<Solution> &solutions_;
};
} // End anonymous namespace

Solution BlackboxOptimizer::computeCoarsening(const vector<Solution> &solutions) {
  assert (solutions.size() >= 1);
  Index nNodes = solutions.front().nNodes();
  unordered_map<Index, Index, SolutionHasher, SolutionComparer> coarseningMap(nNodes, SolutionHasher(solutions), SolutionComparer(solutions));
  coarseningMap.reserve(nNodes);

  Index nCoarsenedNodes = 0;
  vector<Index> coarsening(nNodes);
  for (Index node = 0; node < nNodes; ++node) {
    auto p = coarseningMap.emplace(node, nCoarsenedNodes);
    if (p.second) {
      coarsening[node] = nCoarsenedNodes++;
    }
    else {
      coarsening[node] = p.first->second;
    }
  }

  return Solution(coarsening);
}

void BlackboxOptimizer::runVCycle() {
  if (params_.verbosity >= 3) {
    for (int i = 0; i <= level_; ++i) cout << "  ";
    cout << "V-cycle step with " << hypergraph_.nNodes() << " nodes on " << solutions_.size() << " solutions" << endl;
  }

  shuffle(solutions_.begin(), solutions_.end(), rgen_);

  // Run local search on the initial solutions and add them to the pool until either:
  //    * the new coarsening is large enough: go to the next level
  //    * we used all solutions in the pool: go to the next level
  //    * the coarsening is too large: stop the recursion
  for (size_t nSols = 1; nSols <= solutions_.size(); ++nSols) {
    localSearch_.run(hypergraph_, solutions_[nSols-1], params_, rgen_);
    Solution coarsening = computeCoarsening(vector<Solution>(solutions_.begin(), solutions_.begin() + nSols));
    if (coarsening.nParts() > hypergraph_.nNodes() / params_.minCoarseningFactor) {
      // No success in coarsening anymore; time to stop the cycle
      return;
    }
    if (nSols == solutions_.size() || coarsening.nParts() > hypergraph_.nNodes() / params_.maxCoarseningFactor) {
      // New coarsening large enough: apply and recurse
      Hypergraph cHypergraph = hypergraph_.coarsen(coarsening);
      vector<Solution> cSolutions;
      for (size_t i = 0; i < nSols; ++i) {
        cSolutions.emplace_back(solutions_[i].coarsen(coarsening));
      }
      BlackboxOptimizer nextLevel(cHypergraph, params_, localSearch_, rgen_, cSolutions, level_+1);
      nextLevel.runVCycle();
      for (size_t i = 0; i < nSols; ++i) {
        solutions_[i] = cSolutions[i].uncoarsen(coarsening);
        localSearch_.run(hypergraph_, solutions_[i], params_, rgen_);
      }
      break;
    }
  }

  checkConsistency();
  if (params_.verbosity >= 3) {
    for (int i = 0; i <= level_; ++i) cout << "  ";
    cout << "V-cycle step done" << endl;
  }
}

void BlackboxOptimizer::checkConsistency() const {
  hypergraph_.checkConsistency();
  for (const Solution &solution : solutions_) {
    if (hypergraph_.nNodes() != solution.nNodes())
      throw runtime_error("Hypergraph and solutions must have the same number of nodes");
    if (hypergraph_.nParts() != solution.nParts())
      throw runtime_error("Hypergraph and solutions must have the same number of partitions");
    solution.checkConsistency();
  }
}

} // End namespace minipart


