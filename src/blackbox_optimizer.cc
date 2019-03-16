// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "blackbox_optimizer.hh"
#include "incremental_solution.hh"

#include <iostream>
#include <unordered_map>
#include <cassert>
#include <algorithm>
#include <limits>

using namespace std;

namespace minipart {
BlackboxOptimizer::BlackboxOptimizer(const Hypergraph &hypergraph, const Params &params, std::mt19937 &rgen, std::vector<Solution> &solutions, Index level)
: hypergraph_(hypergraph)
, params_(params)
, rgen_(rgen)
, solutions_(solutions)
, level_(level) {
}

Solution BlackboxOptimizer::runInitialPlacement(const Hypergraph &hypergraph, const Params &params, mt19937 &rgen) {
  uniform_int_distribution<int> partDist(0, hypergraph.nParts()-1);
  Solution solution(hypergraph.nNodes(), hypergraph.nParts());
  for (Index i = 0; i < hypergraph.nNodes(); ++i) {
    solution[i] = partDist(rgen);
  }
  return solution;
}

void BlackboxOptimizer::report() const {
  if (solutions_.empty()) {
    cout << "No solution in the pool" << endl;
    return;
  }

  Index nValidSols = 0;
  Index bestOverflow = numeric_limits<Index>::max();
  Index bestValidConnectivity = 0;

  for (const Solution &solution : solutions_) {
    Index ovf = hypergraph_.metricsSumOverflow(solution);
    Index conn = hypergraph_.metricsConnectivity(solution);
    if (ovf == 0) {
      if (nValidSols == 0) bestValidConnectivity = conn;
      else bestValidConnectivity = min(conn, bestValidConnectivity);
      bestOverflow = 0;
      ++nValidSols;
    }
    else {
      bestOverflow = min(ovf, bestOverflow);
    }
  }

  //cout << solutions_.size() << " solutions" << endl;
  if (nValidSols > 0) {
    cout << nValidSols << " valid solutions" << endl;
    cout << "Best solution has connectivity " << bestValidConnectivity << endl;
  }
  else {
    cout << "Best solution is not valid: overflow " << bestOverflow << endl;
  }
}

Solution BlackboxOptimizer::run(const Hypergraph &hypergraph, const Params &params) {
  mt19937 rgen(params.seed);
  vector<Solution> solutions;
  for (int i = 0; i < params.nSolutions; ++i) {
    solutions.push_back(runInitialPlacement(hypergraph, params, rgen));
  }

  for (int i = 0; i < params.nCycles; ++i) {
    BlackboxOptimizer opt(hypergraph, params, rgen, solutions, 0);
    opt.runVCycle();
    opt.report();
  }

  Index bestOverflow = numeric_limits<Index>::max();
  Index bestConnectivity = numeric_limits<Index>::max();
  size_t best = 0;
  for (size_t i = 0; i < solutions.size(); ++i) {
    Index ovf = hypergraph.metricsSumOverflow(solutions[i]);
    if (ovf > bestOverflow)
      continue;

    Index conn = hypergraph.metricsConnectivity(solutions[i]);
    if (ovf < bestOverflow || conn < bestConnectivity) {
      bestOverflow = ovf;
      bestConnectivity = conn;
      best = i;
    }
  }

  return solutions[best];
}

void BlackboxOptimizer::runLocalSearch(Solution &solution) {
  IncrementalSolution inc(hypergraph_, solution);
  Index nMoves = params_.movesPerElement * inc.nNodes() * (inc.nParts()-1);
  runMovePass(inc, nMoves, rgen_);
  runSwapPass(inc, nMoves, rgen_);
}

void BlackboxOptimizer::runMovePass(IncrementalSolution &inc, Index nMoves, mt19937 &rgen) {
  uniform_int_distribution<int> partDist(0, inc.nParts()-1);
  uniform_int_distribution<int> nodeDist(0, inc.nNodes()-1);
  for (int iter = 0; iter < nMoves; ++iter) {
    Index overflow = inc.metricsSumOverflow();
    Index cost = inc.metricsSoed();
    Index node = nodeDist(rgen);
    Index src = inc.solution(node);
    Index dst = partDist(rgen);
    inc.move(node, dst);
    if (inc.metricsSumOverflow() > overflow ||
        (inc.metricsSumOverflow() == overflow && inc.metricsSoed() > cost)) {
      inc.move(node, src);
    }
  }
}

void BlackboxOptimizer::runSwapPass(IncrementalSolution &inc, Index nMoves, mt19937 &rgen) {
  uniform_int_distribution<int> nodeDist(0, inc.nNodes()-1);
  for (int iter = 0; iter < nMoves; ++iter) {
    Index overflow = inc.metricsSumOverflow();
    Index cost = inc.metricsSoed();
    Index n1 = nodeDist(rgen);
    Index n2 = nodeDist(rgen);
    Index p1 = inc.solution(n1);
    Index p2 = inc.solution(n1);
    if (p1 == p2) continue;
    inc.move(n1, p2);
    inc.move(n2, p1);
    if (inc.metricsSumOverflow() > overflow ||
        (inc.metricsSumOverflow() == overflow && inc.metricsSoed() > cost)) {
      inc.move(n1, p1);
      inc.move(n2, p2);
    }
  }
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
  for (int i = 0; i < level_; ++i) cout << "  ";
  cout << "V-cycle step with " << hypergraph_.nNodes() << " nodes on " << solutions_.size() << " solutions" << endl;

  shuffle(solutions_.begin(), solutions_.end(), rgen_);

  // Run local search on the initial solutions and add them to the pool until either:
  //    * the new coarsening is large enough: go to the next level
  //    * we used all solutions in the pool: go to the next level
  //    * the coarsening is too large: stop the recursion
  for (size_t nSols = 1; nSols <= solutions_.size(); ++nSols) {
    runLocalSearch(solutions_[nSols-1]);
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
      BlackboxOptimizer nextLevel(cHypergraph, params_, rgen_, cSolutions, level_+1);
      nextLevel.runVCycle();
      for (size_t i = 0; i < nSols; ++i) {
        solutions_[i] = cSolutions[i].uncoarsen(coarsening);
        runLocalSearch(solutions_[i]);
      }
      break;
    }
  }

  checkConsistency();
  for (int i = 0; i < level_; ++i) cout << "  ";
  cout << "V-cycle step done" << endl;
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


