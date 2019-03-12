
#include "blackbox_optimizer.hh"
#include "incremental_solution.hh"

#include <iostream>
#include <unordered_map>
#include <cassert>

using namespace std;

namespace minipart {
vector<Index> BlackboxOptimizer::runInitialPlacement(const Hypergraph &hypergraph, const Params &params, mt19937 &rgen) {
  uniform_int_distribution<int> partDist(0, params.nPartitions-1);
  vector<Index> solution(hypergraph.nNodes());
  for (Index &p : solution) {
    p = partDist(rgen);
  }
  return solution;
}

vector<Index> BlackboxOptimizer::computePartitionCapacities(const Hypergraph &hypergraph, const Params &params) {
  Index totalCapacity = hypergraph.totalNodeWeight() * (1.0 + params.imbalanceFactor);
  Index partitionCapacity = totalCapacity  / params.nPartitions;
  vector<Index> partitionCapacities(params.nPartitions, partitionCapacity);
  partitionCapacities[0] = totalCapacity - partitionCapacity * (params.nPartitions - 1);
  return partitionCapacities;
}

vector<Index> BlackboxOptimizer::run(const Hypergraph &hypergraph, const Params &params) {
  mt19937 rgen(params.seed);
  vector<Index> partitionCapacities = computePartitionCapacities(hypergraph, params);

  //vector<Index> solution = runInitialPlacement(hypergraph, params, rgen);
  //cout << "Initial cut: " << hypergraph.metricsCut(solution) << endl;
  //cout << "Initial connectivity: " << hypergraph.metricsConnectivity(solution) << endl;
  //runLocalSearch(hypergraph, params, partitionCapacities, rgen, solution);
  //cout << "Final cut: " << hypergraph.metricsCut(solution) << endl;
  //cout << "Final connectivity: " << hypergraph.metricsConnectivity(solution) << endl;
  //cout << endl;

  vector<vector<Index> > solutions;
  for (int i = 0; i < 32; ++i) {
    vector<Index> solution = runInitialPlacement(hypergraph, params, rgen);
    cout << "Initial cut: " << hypergraph.metricsCut(solution) << endl;
    cout << "Initial connectivity: " << hypergraph.metricsConnectivity(solution) << endl;
    runLocalSearch(hypergraph, params, partitionCapacities, rgen, solution);
    cout << "Final cut: " << hypergraph.metricsCut(solution) << endl;
    cout << "Final connectivity: " << hypergraph.metricsConnectivity(solution) << endl;
    solutions.push_back(solution);
    computeCoarsening(solutions);
    cout << endl;
  }
  return solutions.front();
}

void BlackboxOptimizer::runLocalSearch(const Hypergraph &hypergraph, const Params &params, const vector<Index> &partitionCapacities, mt19937 &rgen, vector<Index> &solution) {
  uniform_int_distribution<int> partDist(0, params.nPartitions-1);
  uniform_int_distribution<int> nodeDist(0, hypergraph.nNodes()-1);
  IncrementalSolution inc(hypergraph, solution, partitionCapacities);
  for (int iter = 0; iter < params.movesPerElement * hypergraph.nNodes() * (params.nPartitions-1); ++iter) {
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
  SolutionHasher(const vector<vector<Index> > &solutions)
    : solutions_(solutions) {}

  uint64_t operator()(const Index &node) const {
    // FNV hash
    uint64_t magic = 1099511628211llu;
    uint64_t ret = 0;
    for (const vector<Index> &solution : solutions_) {
      ret = (ret ^ (uint64_t)solution[node]) * magic;
    }
    return ret;
  }

 private:
  const vector<vector<Index> > &solutions_;
};

class SolutionComparer {
 public:
  SolutionComparer(const vector<vector<Index> > &solutions)
    : solutions_(solutions) {}

  bool operator()(const Index &n1, const Index &n2) const {
    for (const vector<Index> &solution : solutions_) {
      if (solution[n1] != solution[n2]) return false;
    }
    return true;
  }

 private:
  const vector<vector<Index> > &solutions_;
};
} // End anonymous namespace

vector<Index> BlackboxOptimizer::computeCoarsening(const vector<vector<Index> > &solutions) {
  assert (solutions.size() >= 1);
  Index nNodes = solutions.front().size();
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
  return coarsening;
}
} // End namespace minipart


