// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_HYPERGRAPH_HH
#define MINIPART_HYPERGRAPH_HH

#include "common.hh"
#include "solution.hh"
#include <iosfwd>

namespace minipart {

class Hypergraph {
 public:
  Index nNodes  () const { return nodeWeights_.size(); }
  Index nHedges () const { return hedgeWeights_.size(); }
  Index nPins() const;
  Index nParts() const { return partWeights_.size(); }

  Index totalNodeWeight() const;
  Index totalHedgeWeight() const;
  Index totalPartWeight() const;

  const std::vector<Index> &hedgeNodes(Index hedge) const { return hedgeToNodes_[hedge]; }
  const std::vector<Index> &nodeHedges(Index node) const { return nodeToHedges_[node]; }

  Index hedgeWeight (Index hedge) const { return hedgeWeights_[hedge]; }
  Index nodeWeight  (Index node)  const { return nodeWeights_[node]; }
  Index partWeight  (Index part)  const { return partWeights_[part]; }

  // Metrics
  Index metricsSumOverflow(const Solution &solution) const;
  Index metricsEmptyPartitions(const Solution &solution) const;
  Index metricsCut(const Solution &solution) const;
  Index metricsSoed(const Solution &solution) const;
  Index metricsConnectivity(const Solution &solution) const;
  Index metricsMaxDegree(const Solution &solution) const;
  Index metricsDaisyChainDistance(const Solution &solution) const;
  Index metricsDaisyChainMaxDegree(const Solution &solution) const;
  double metricsRatioCut(const Solution &solution) const;
  double metricsRatioSoed(const Solution &solution) const;
  double metricsRatioConnectivity(const Solution &solution) const;
  double metricsRatioMaxDegree(const Solution &solution) const;

  double metricsRatioPenalty(const Solution &solution) const;
  std::vector<Index> metricsPartitionUsage(const Solution &solution) const;
  std::vector<Index> metricsPartitionDegree(const Solution &solution) const;
  std::vector<Index> metricsPartitionDaisyChainDegree(const Solution &solution) const;

  // IO functions
  static Hypergraph readHgr(std::istream &);
  void writeHgr(std::ostream &) const;

  // Coarsening
  Hypergraph coarsen(const Solution &coarsening) const;

  // Modifications
  void setupPartitions(Index nParts, double imbalanceFactor);
  void mergeParallelHedges();

  void checkConsistency() const; 

 private:
  void constructNodes();
  void constructHedges();

  bool cut(const Solution &solution, Index hedge) const;
  Index degree(const Solution &solution, Index hedge) const;

 private:
  std::vector<Index> nodeWeights_;
  std::vector<Index> hedgeWeights_;
  std::vector<Index> partWeights_;
  std::vector<std::vector<Index> > nodeToHedges_;
  std::vector<std::vector<Index> > hedgeToNodes_;
};

} // End namespace minipart

#endif

