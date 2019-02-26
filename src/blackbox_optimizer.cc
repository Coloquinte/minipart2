
#include "blackbox_optimizer.hh"
#include "incremental_solution.hh"

#include <iostream>

using namespace std;

namespace minipart {
vector<Index> BlackboxOptimizer::run(const Hypergraph &hypergraph, const Params &params) {
  mt19937 rgen(params.seed);
  uniform_int_distribution<int> partDist(0, params.nPartitions-1);
  uniform_int_distribution<int> nodeDist(0, hypergraph.nNodes()-1);

  Index totalCapacity = hypergraph.totalNodeWeight() * (1.0 + params.imbalanceFactor);
  Index partitionCapacity = totalCapacity  / params.nPartitions;
  vector<Index> partitionCapacities(params.nPartitions, partitionCapacity);
  partitionCapacities[0] = totalCapacity - partitionCapacity * (params.nPartitions - 1);

  // Initial solution
  vector<Index> solution(hypergraph.nNodes());
  for (Index &p : solution) {
    p = partDist(rgen);
  }

  // Local search pass
  IncrementalSolution inc(hypergraph, solution, partitionCapacities);
  cout << "Initial: " << inc.metricsSoed() << endl;
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
  cout << "Final: " << inc.metricsSoed() << endl;

  return solution;
}
} // End namespace minipart


