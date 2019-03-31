// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_INCREMENTAL_SOLUTION_HH
#define MINIPART_INCREMENTAL_SOLUTION_HH

#include "hypergraph.hh"

namespace minipart {

class IncrementalSolution {
 public:
  IncrementalSolution(const Hypergraph &hypergraph, Solution &solution);

  void move(Index node, Index to);

  Index nNodes() const { return hypergraph_.nNodes(); }
  Index nHedges() const { return hypergraph_.nHedges(); }
  Index nParts() const { return hypergraph_.nParts(); }
  const Hypergraph &hypergraph() const { return hypergraph_; }

  const Solution& solution() const { return solution_; }
  Index solution(Index node) const { return solution_[node]; }

  Index demand(Index partition) const { return partitionDemands_[partition]; }
  Index capacity(Index partition) const { return hypergraph_.partWeight(partition); }

  // Metrics
  Index metricsCut() const { return currentCut_; }
  Index metricsSoed() const { return currentSoed_; }
  Index metricsConnectivity() const { return currentSoed_ - nHedges(); }
  Index metricsSumOverflow() const;
  Index metricsMaxDegree() const;

  void checkConsistency() const;

 private:
  bool cut(Index hedge) const { return hedgeDegrees_[hedge] > 1; }
  Index degree(Index hedge) const { return hedgeDegrees_[hedge]; }

 private:
  std::vector<Index> computePartitionDemands() const;
  std::vector<std::vector<Index> > computeHedgeNbPinsPerPartition() const;
  std::vector<Index> computeHedgeDegrees() const;
  std::vector<Index> computePartitionDegrees() const;
  Index computeCut() const;
  Index computeSoed() const;

 private:
  const Hypergraph &hypergraph_;
  Solution &solution_;

  // Partition state
  std::vector<Index> partitionDemands_;

  // Hyperedge states
  std::vector<std::vector<Index> > hedgeNbPinsPerPartition_;
  std::vector<Index> hedgeDegrees_;
  std::vector<Index> partitionDegrees_;
  Index currentCut_;
  Index currentSoed_;
};

} // End namespace minipart

#endif

