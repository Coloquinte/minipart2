// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "blackbox_optimizer.hh"
#include "incremental_solution.hh"

#include <iostream>
#include <unordered_map>
#include <cassert>

using namespace std;

namespace minipart {
Solution BlackboxOptimizer::runInitialPlacement(const Hypergraph &hypergraph, const Params &params, mt19937 &rgen) {
  uniform_int_distribution<int> partDist(0, hypergraph.nParts()-1);
  Solution solution(hypergraph.nNodes(), hypergraph.nParts());
  for (Index i = 0; i < hypergraph.nNodes(); ++i) {
    solution[i] = partDist(rgen);
  }
  return solution;
}

Solution BlackboxOptimizer::run(const Hypergraph &hypergraph, const Params &params) {
  mt19937 rgen(params.seed);
  vector<Solution> solutions;
  runVCycle(hypergraph, params, rgen, solutions);

  /*
  for (int i = 0; i < 32; ++i) {
    vector<Index> solution = runInitialPlacement(hypergraph, params, rgen);
    cout << "Initial cut: " << hypergraph.metricsCut(solution) << endl;
    cout << "Initial connectivity: " << hypergraph.metricsConnectivity(solution) << endl;
    runLocalSearch(hypergraph, params, rgen, solution);
    cout << "Final cut: " << hypergraph.metricsCut(solution) << endl;
    cout << "Final connectivity: " << hypergraph.metricsConnectivity(solution) << endl;
    solutions.push_back(solution);
    computeCoarsening(solutions);
    cout << endl;
  }
  */
  return solutions.front();
}

void BlackboxOptimizer::runLocalSearch(const Hypergraph &hypergraph, const Params &params, mt19937 &rgen, Solution &solution) {
  uniform_int_distribution<int> partDist(0, hypergraph.nParts()-1);
  uniform_int_distribution<int> nodeDist(0, hypergraph.nNodes()-1);
  IncrementalSolution inc(hypergraph, solution);
  for (int iter = 0; iter < params.movesPerElement * hypergraph.nNodes() * (hypergraph.nParts()-1); ++iter) {
    Index overflow = inc.metricsSumOverflow();
    Index cost = inc.metricsSoed();
    Index node = nodeDist(rgen);
    Index src = inc.solution(node);
    Index dst = partDist(rgen);
    inc.move(node, dst);
    if (inc.metricsSumOverflow() > overflow ||
        (inc.metricsSumOverflow() == overflow && inc.metricsSoed() > cost))
      inc.move(node, src);
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

  cout << "Coarsening: " << nCoarsenedNodes << " nodes" << endl;
  return Solution(coarsening);
}

void BlackboxOptimizer::runVCycle(const Hypergraph &hypergraph, const Params &params, mt19937 &rgen, vector<Solution> &solutions) {
  // Order the solutions by quality
  // TODO
  
  // Run local search on the initial solutions and add them to the pool until either:
  //    * the new coarsening is large enough
  //    * a termination condition is reached
  vector<Solution> pool;
  const int maxNbSols = 32;
  for (size_t nbSols = 1; nbSols <= maxNbSols; ++nbSols) {
    if (solutions.size() < nbSols)
      pool.push_back(runInitialPlacement(hypergraph, params, rgen));
    else
      pool.push_back(solutions[nbSols-1]);
    runLocalSearch(hypergraph, params, rgen, pool.back());
    // TODO: check current coarsening
    computeCoarsening(pool);
  }

  // Apply coarsening and recurse
  // TODO
  
  // Rerun local search on the solutions we got
  solutions = pool;
}
} // End namespace minipart


