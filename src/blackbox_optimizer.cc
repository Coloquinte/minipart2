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

  //cout << "Coarsening: " << nCoarsenedNodes << " nodes" << endl;
  return Solution(coarsening);
}

void BlackboxOptimizer::runVCycle(const Hypergraph &hypergraph, const Params &params, mt19937 &rgen, vector<Solution> &solutions) {
  cout << "V-cycle step with " << hypergraph.nNodes() << " nodes on " << solutions.size() << " solutions" << endl;
  // Order the solutions by quality
  // TODO
  
  // Run local search on the initial solutions and add them to the pool until either:
  //    * the new coarsening is large enough
  //    * a termination condition is reached
  const int maxNbSols = 32;
  for (size_t nSols = 1; nSols <= maxNbSols; ++nSols) {
    if (solutions.size() < nSols)
      solutions.push_back(runInitialPlacement(hypergraph, params, rgen));
    runLocalSearch(hypergraph, params, rgen, solutions[nSols-1]);
    Solution coarsening = computeCoarsening(vector<Solution>(solutions.begin(), solutions.begin() + nSols));
    if (coarsening.nParts() == coarsening.nNodes()) {
      // No success in coarsening anymore; time to stop the cycle
      return;
    }
    if (coarsening.nParts() > hypergraph.nNodes() / params.coarseningFactor) {
      // New coarsening large enough: apply and recurse
      Hypergraph cHypergraph = hypergraph.coarsen(coarsening);
      vector<Solution> cSolutions;
      for (size_t i = 0; i < nSols; ++i) {
        cSolutions.emplace_back(solutions[i].coarsen(coarsening));
      }
      runVCycle(cHypergraph, params, rgen, cSolutions);
      solutions.clear();
      for (const Solution &cSolution : cSolutions) {
        solutions.emplace_back(cSolution.uncoarsen(coarsening));
      }
      break;
    }
  }

  cout << "Reoptimization step with " << hypergraph.nNodes() << " nodes on " << solutions.size() << " solutions" << endl;
  // Rerun local search on the solutions we got back
  for (Solution &solution : solutions) {
    runLocalSearch(hypergraph, params, rgen, solution);
  }
}
} // End namespace minipart


