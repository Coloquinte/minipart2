// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_HYPERGRAPH_HH
#define MINIPART_HYPERGRAPH_HH

#include "common.hh"
#include "solution.hh"
#include <iosfwd>

namespace minipart {

template<typename T>
class Range {
 public:
  Range(const T *b, const T *e) : begin_(b), end_(e) {}
  const T *begin() const { return begin_; }
  const T *end() const { return end_; }
  const T *cbegin() const { return begin_; }
  const T *cend() const { return end_; }
  std::size_t size() const { return end_ - begin_; }

  bool operator==(const Range<T> &o) {
    if (size() != o.size()) return false;
    for (std::size_t i = 0, sz = size(); i != sz; ++i) {
      if (begin_[i] != o.begin_[i]) return false;
    }
    return true;
  }
  bool operator!=(const Range<T> &o) { return !operator==(o); }

 private:
  const T *begin_;
  const T *end_;
};

class Hypergraph {
 public:
  Hypergraph(Index nodeWeights=1, Index hedgeWeights=1, Index partWeights=1);

  Index nNodes  () const { return nNodes_; }
  Index nHedges () const { return nHedges_; }
  Index nParts  () const { return nParts_; }
  Index nPins   () const { return nPins_; }

  Index nNodeWeights  () const { return nNodeWeights_; }
  Index nHedgeWeights () const { return nHedgeWeights_; }
  Index nPartWeights  () const { return nPartWeights_; }

  Index totalNodeWeight  (Index i=0) const { return totalNodeWeights_[i]; }
  Index totalHedgeWeight (Index i=0) const { return totalHedgeWeights_[i]; }
  Index totalPartWeight  (Index i=0) const { return totalPartWeights_[i]; }

  Index nodeWeight  (Index node,  Index i=0) const { return nodeData_[nodeBegin_[node] + i]; }
  Index hedgeWeight (Index hedge, Index i=0) const { return hedgeData_[hedgeBegin_[hedge] + i]; }
  Index partWeight  (Index part,  Index i=0) const { return partData_[part * nPartWeights_ + i]; }

  Range<Index> hedgeNodes(Index hedge) const {
    const Index *ptr = hedgeData_.data();
    Index b = hedgeBegin_[hedge] + nHedgeWeights_;
    Index e = hedgeBegin_[hedge+1];
    return Range<Index>(ptr + b, ptr + e);
  }

  Range<Index> nodeHedges(Index node) const {
    const Index *ptr = nodeData_.data();
    Index b = nodeBegin_[node] + nNodeWeights_;
    Index e = nodeBegin_[node+1];
    return Range<Index>(ptr + b, ptr + e);
  }

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
  static Hypergraph readFile(const std::string &name);
  void writeFile(const std::string &name) const;
  static Hypergraph readHgr(std::istream &);
  void writeHgr(std::ostream &) const;
  static Hypergraph readMinipart(std::istream &);
  void writeMinipart(std::ostream &) const;

  // Coarsening
  Hypergraph coarsen(const Solution &coarsening) const;

  // Modifications
  void setupBlocks(Index nParts, double imbalanceFactor);
  void mergeParallelHedges();

  void checkConsistency() const; 

 private:
  void finalize();
  void finalizePins();
  void finalizeNodes();
  void finalizeNodeWeights();
  void finalizeHedgeWeights();
  void finalizePartWeights();

  bool cut(const Solution &solution, Index hedge) const;
  Index degree(const Solution &solution, Index hedge) const;

  static Hypergraph readStream(const std::string &name, std::istream &);
  void writeStream(const std::string &name, std::ostream &) const;

 private:
  // Basic stats
  Index nNodes_;
  Index nHedges_;
  Index nParts_;
  Index nPins_;

  // Number of resources for each of them
  Index nNodeWeights_;
  Index nHedgeWeights_;
  Index nPartWeights_;

  // Begin/end in the compressed representation
  std::vector<Index> nodeBegin_;
  std::vector<Index> hedgeBegin_;

  // Data, with the weights then the pins for each node/edge
  std::vector<Index> nodeData_;
  std::vector<Index> hedgeData_;
  std::vector<Index> partData_;

  // Summary stats
  std::vector<Index> totalNodeWeights_;
  std::vector<Index> totalHedgeWeights_;
  std::vector<Index> totalPartWeights_;
};

} // End namespace minipart

#endif

