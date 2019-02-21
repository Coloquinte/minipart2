// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_HYPERGRAPH_HH
#define MINIPART_HYPERGRAPH_HH

#include <cstdint>
#include <vector>
#include <iosfwd>

namespace minipart {

typedef std::int32_t Index;

class Hypergraph {
 public:
  Index nNodes  () const { return nodeWeights_.size(); }
  Index nHedges () const { return hedgeWeights_.size(); }
  Index nPins() const;

  const std::vector<Index> hedgeNodes(Index hedge) const { return hedgeToNodes_[hedge]; }
  const std::vector<Index> nodeHedges(Index node) const { return nodeToHedges_[node]; }

  Index hedgeWeight (Index hedge) const { return hedgeWeights_[hedge]; }
  Index nodeWeight  (Index node)  const { return nodeWeights_[node]; }

  Hypergraph coarsen(const std::vector<Index> &coarsening) const;
  void mergeParallelHedges();

  // Metrics
  Index metricsCut(const std::vector<Index> &solution) const;
  Index metricsSoed(const std::vector<Index> &solution) const;
  Index metricsConnectivity(const std::vector<Index> &solution) const;

  // IO functions
  static Hypergraph readHmetis(std::istream &);
  void writeHmetis(std::ostream &) const;

  void checkConsistency() const; 

 private:
  void constructNodes();
  void constructHedges();

  bool cut(const std::vector<Index> &solution, Index hedge) const;
  Index degree(const std::vector<Index> &solution, Index hedge) const;

 private:
  std::vector<Index> nodeWeights_;
  std::vector<Index> hedgeWeights_;
  std::vector<std::vector<Index> > nodeToHedges_;
  std::vector<std::vector<Index> > hedgeToNodes_;
};

} // End namespace minipart

#endif

