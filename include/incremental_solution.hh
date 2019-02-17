// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_INCREMENTAL_SOLUTION_HH
#define MINIPART_INCREMENTAL_SOLUTION_HH

#include "hypergraph.hh"

namespace minipart {

class IncrementalSolution {
 public:
  IncrementalSolution(const Hypergraph &hypergraph, const std::vector<Index> &solution, const std::vector<Index> &partitionCapacities);

  void move(Index node, Index to);

  Index nNodes() const { return hypergraph_.nNodes(); }
  Index nHedges() const { return hypergraph_.nHedges(); }
  Index nPartitions() const { return partitionCapacities_.size(); }

  const std::vector<Index>& solution() const { return solution_; }
  Index solution(Index node) const { return solution_[node]; }

  bool cut(Index hedge) const { return hedgeDegrees_[hedge] != 0; }
  Index degree(Index hedge) const { return hedgeDegrees_[hedge]; }

  Index demand(Index partition) const { return partitionDemands_[partition]; }
  Index capacity(Index partition) const { return partitionCapacities_[partition]; }

  // Metrics
  Index metricsCut() const { return currentCut_; }
  Index metricsSoed() const { return currentSoed_; }
  Index metricsSumOverflow() const;

 private:
  std::vector<Index> computePartitionDemands() const;
  std::vector<std::vector<Index> > computeHedgeNbPinsPerPartition() const;
  std::vector<Index> computeHedgeDegrees() const;
  Index computeCut() const;
  Index computeSoed() const;

 private:
  const Hypergraph &hypergraph_;
  std::vector<Index> solution_;

  // Partition states
  std::vector<Index> partitionDemands_;
  std::vector<Index> partitionCapacities_;

  // Hyperedge states
  std::vector<std::vector<Index> > hedgeNbPinsPerPartition_;
  std::vector<Index> hedgeDegrees_;
  Index currentCut_;
  Index currentSoed_;
};

} // End namespace minipart

#endif

