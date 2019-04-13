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

void BlackboxOptimizer::report(const string &step) const {
  report(step, solutions_.size());
}

void BlackboxOptimizer::report(const string &step, Index nSols) const {
  if (params_.verbosity >= 3) {
    for (int i = 0; i < level_; ++i) cout << "  ";
    cout << step << ": ";
    cout << hypergraph_.nNodes() << " nodes, ";
    cout << hypergraph_.nHedges() << " edges, ";
    cout << hypergraph_.nPins() << " pins ";
    cout << "on " << nSols << " solutions";
    cout << endl;
  }
}

Solution BlackboxOptimizer::run(const Hypergraph &hypergraph, const PartitioningParams &params, const LocalSearch &localSearch) {
  mt19937 rgen(params.seed);
  vector<Solution> sols;
  BlackboxOptimizer opt(hypergraph, params, localSearch, rgen, sols, 0);
  return opt.run();
}

Solution BlackboxOptimizer::run() {
  runInitialPlacement();
  runLocalSearch();
  for (int i = 0; i < params_.nCycles; ++i) {
    if (params_.verbosity >= 2)
      cout << "Starting V-cycle #" << i + 1 << endl;
    runVCycle();
    if (params_.verbosity >= 2) {
      Solution solution = bestSolution();
      localSearch_.report(hypergraph_, solution, cout);
      cout << endl;
    }
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

namespace {

class CoarseningComparer {
 public:
  CoarseningComparer(const PartitioningParams &params)
  : params_(params) {
  }

  bool operator()(const Solution &c1, const Solution &c2) const {
    assert (c1.nNodes() == c2.nNodes());
    Index nNodes = c1.nNodes();
    double fac1 = nNodes / (double) c1.nParts();
    double fac2 = nNodes / (double) c2.nParts();
    if (fac1 < params_.minCoarseningFactor || fac2 < params_.minCoarseningFactor)
      return fac1 > fac2;
    if (fac1 > params_.maxCoarseningFactor || fac2 < params_.maxCoarseningFactor)
      return fac1 < fac2;
    double tgt = 0.5 * (params_.maxCoarseningFactor + params_.minCoarseningFactor);
    return abs(fac1 - tgt) < abs(fac2 - tgt);
  }

 private:
  const PartitioningParams &params_;
};
} // End anonymous namespace

void BlackboxOptimizer::runLocalSearch() {
  report ("Local search");
  for (Solution &solution : solutions_) {
    localSearch_.run(hypergraph_, solution, params_, rgen_);
  }
}

void BlackboxOptimizer::runVCycle() {
  checkConsistency();

  if (hypergraph_.nNodes() < params_.minCoarseningNodes * hypergraph_.nParts()) return;
  report ("V-cycle step");

  // Pick the best number of solutions for the coarsening
  // If the coarsening is still too large, stop the recursion
  shuffle(solutions_.begin(), solutions_.end(), rgen_);

  vector<Solution> coarsenings;
  for (size_t nSols = 1; nSols <= solutions_.size(); ++nSols) {
    coarsenings.emplace_back(computeCoarsening(vector<Solution>(solutions_.begin(), solutions_.begin() + nSols)));
  }
  size_t coarseningIndex  = min_element(coarsenings.begin(), coarsenings.end(), CoarseningComparer(params_)) - coarsenings.begin();
  Solution coarsening = coarsenings[coarseningIndex];
  if (coarsening.nNodes() / (double) coarsening.nParts() < params_.minCoarseningFactor) return;

  Hypergraph cHypergraph = hypergraph_.coarsen(coarsening);
  vector<Solution> cSolutions;
  for (size_t i = 0; i <= coarseningIndex; ++i) {
    cSolutions.emplace_back(solutions_[i].coarsen(coarsening));
  }
  BlackboxOptimizer nextLevel(cHypergraph, params_, localSearch_, rgen_, cSolutions, level_+1);
  nextLevel.runLocalSearch();
  nextLevel.runVCycle();
  report("Refinement", coarseningIndex + 1);
  for (size_t i = 0; i <= coarseningIndex; ++i) {
    solutions_[i] = cSolutions[i].uncoarsen(coarsening);
    localSearch_.run(hypergraph_, solutions_[i], params_, rgen_);
  }
  checkConsistency();
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


