// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "hypergraph.hh"

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <cmath>

using namespace std;

namespace minipart {

Hypergraph::Hypergraph(Index nodeWeights, Index hedgeWeights, Index partWeights) {
  nNodes_ = 0;
  nHedges_ = 0;
  nParts_ = 0;
  nNodeWeights_ = nodeWeights;
  nHedgeWeights_ = hedgeWeights;
  nPartWeights_ = partWeights;
  totalNodeWeights_.assign(nodeWeights, 0);
  totalHedgeWeights_.assign(hedgeWeights, 0);
  totalPartWeights_.assign(partWeights, 0);
  hedgeBegin_.push_back(0);
  nodeBegin_.push_back(0);
}

Hypergraph Hypergraph::coarsen(const Solution &coarsening) const {
  assert (nNodes() == coarsening.nNodes());
  assert (coarsening.nParts() <= nNodes());
  assert (coarsening.nParts() > 0);
  if (nNodes() == 0) return *this;

  Hypergraph ret(nNodeWeights_, nHedgeWeights_, nPartWeights_);
  ret.nNodes_ = coarsening.nParts();
  ret.nHedges_ = 0;
  ret.nParts_ = nParts_;

  // Hyperedges
  vector<Index> pins;
  for (Index hedge = 0; hedge < nHedges_; ++hedge) {
    for (Index node : hedgeNodes(hedge)) {
      pins.push_back(coarsening[node]);
    }
    sort(pins.begin(), pins.end());
    pins.resize(unique(pins.begin(), pins.end()) - pins.begin());
    if (pins.size() > 1) {
      for (Index i = 0; i < nHedgeWeights_; ++i) {
        ret.hedgeData_.push_back(hedgeWeight(hedge, i));
      }
      ret.hedgeData_.insert(ret.hedgeData_.end(), pins.begin(), pins.end());
      ret.hedgeBegin_.push_back(ret.hedgeData_.size());
      ++ret.nHedges_;
    }
    pins.clear();
  }

  // Node weights
  ret.nodeData_.assign(coarsening.nParts() * nNodeWeights_, 0);
  for (Index node = 0; node < nNodes_; ++node) {
    Index coarsened = coarsening[node];
    for (Index i = 0; i < nNodeWeights_; ++i) {
      ret.nodeData_[coarsened * nNodeWeights_ + i] += nodeWeight(node, i);
    }
  }
  for (Index i = 1; i <= coarsening.nParts(); ++i) {
    ret.nodeBegin_.push_back(i * nNodeWeights_);
  }

  // Partitions
  ret.partData_ = partData_;

  // Finalize
  ret.mergeParallelHedges();

  assert (ret.totalNodeWeights_ == totalNodeWeights_);
  assert (ret.totalPartWeights_ == totalPartWeights_);

  return ret;
}

void Hypergraph::checkConsistency() const {
  if (nNodes_ < 0) throw runtime_error("Negative number of nodes");
  if (nHedges_ < 0) throw runtime_error("Negative number of hedges");
  if (nParts_ < 0) throw runtime_error("Negative number of parts");
  if (nPins_ < 0) throw runtime_error("Negative number of pins");
  if (nNodeWeights_ < 0) throw runtime_error("Negative number of node weights");
  if (nHedgeWeights_ < 0) throw runtime_error("Negative number of hedge weights");
  if (nPartWeights_ < 0) throw runtime_error("Negative number of part weights");

  if ((Index) nodeBegin_.size() != nNodes_ + 1)
    throw runtime_error("Number of node limits and of nodes do not match");
  if ((Index) hedgeBegin_.size() != nHedges_ + 1)
    throw runtime_error("Number of hedge limits and of hedges do not match");

  for (size_t i = 0; i + 1 < nodeBegin_.size(); ++i) {
    if (nodeBegin_[i] + nNodeWeights_ > nodeBegin_[i+1])
        throw runtime_error("Inconsistent node data");
  }
  if (nodeBegin_.front() != 0) throw runtime_error("Inconsistent node data begin");
  if (nodeBegin_.back() != (Index) nodeData_.size()) throw runtime_error("Inconsistent node data end");

  for (size_t i = 0; i + 1 < hedgeBegin_.size(); ++i) {
    if (hedgeBegin_[i] + nHedgeWeights_ > hedgeBegin_[i+1])
        throw runtime_error("Inconsistent hedge data");
  }
  if (hedgeBegin_.front() != 0) throw runtime_error("Inconsistent hedge data begin");
  if (hedgeBegin_.back() != (Index) hedgeData_.size()) throw runtime_error("Inconsistent hedge data end");

  if (nPins_ + nNodeWeights_ * nNodes_ != (Index) nodeData_.size()) throw runtime_error("Inconsistent node data size");
  if (nPins_ + nHedgeWeights_ * nHedges_ != (Index) hedgeData_.size()) throw runtime_error("Inconsistent hedge data size");
  if (nPartWeights_ * nParts_ != (Index) partData_.size()) throw runtime_error("Inconsistent part data size");

  for (Index n = 0; n != nNodes_; ++n) {
    Range<Index> node = nodeHedges(n);
    for (Index hedge : node) {
      if (hedge < 0 || hedge >= nHedges())
        throw runtime_error("Invalid hedge value");
    }
    unordered_set<Index> uq(node.begin(), node.end());
    if (uq.size() != node.size())
        throw runtime_error("Duplicate hedges in a node");
  }
  for (Index h = 0; h != nHedges_; ++h) {
    Range<Index> hedge = hedgeNodes(h);
    for (Index node : hedge) {
      if (node < 0 || node >= nNodes())
        throw runtime_error("Invalid node value");
    }
    unordered_set<Index> uq(hedge.begin(), hedge.end());
    if (uq.size() != hedge.size())
        throw runtime_error("Duplicate nodes in an hedge");
  }

  // TODO: check that there is a bidirectional mapping between nodes and hedges
}

void Hypergraph::finalize() {
  finalizePins();
  finalizeNodes();
  finalizeNodeWeights();
  finalizeHedgeWeights();
  finalizePartWeights();
  checkConsistency();
}

void Hypergraph::finalizePins() {
  nPins_ = hedgeData_.size() - nHedges_ * nHedgeWeights_;
}

void Hypergraph::finalizeNodeWeights() {
  totalNodeWeights_.assign(nNodeWeights_, 0);
  for (Index i = 0; i < nNodes_; ++i) {
    for (Index j = 0; j < nNodeWeights_; ++j) {
      totalNodeWeights_[j] += nodeWeight(i, j);
    }
  }
}

void Hypergraph::finalizeHedgeWeights() {
  totalHedgeWeights_.assign(nHedgeWeights_, 0);
  for (Index i = 0; i < nHedges_; ++i) {
    for (Index j = 0; j < nHedgeWeights_; ++j) {
      totalHedgeWeights_[j] += hedgeWeight(i, j);
    }
  }
}

void Hypergraph::finalizePartWeights() {
  totalPartWeights_.assign(nPartWeights_, 0);
  for (Index i = 0; i < nParts_; ++i) {
    for (Index j = 0; j < nPartWeights_; ++j) {
      totalPartWeights_[j] += partWeight(i, j);
    }
  }
}

void Hypergraph::finalizeNodes() {
  vector<Index> newData;
  vector<Index> newBegin;
  newBegin.assign(nNodes_ + 1, 0);

  // Setup node begins
  for (Index hedge = 0; hedge < nHedges_; ++hedge) {
    for (Index node : hedgeNodes(hedge)) {
      ++newBegin[node+1];
    }
  }
  for (Index node = 1; node <= nNodes_; ++node) {
    newBegin[node] += newBegin[node-1] + nNodeWeights_;
  }
  newData.resize(newBegin.back());
  assert ((Index) newData.size() == nPins_ + nNodes_ * nNodeWeights_);

  // Assign node weights
  for (Index node = 0; node < nNodes_; ++node) {
    Index b = newBegin[node];
    for (Index i = 0; i < nNodeWeights_; ++i) {
      newData[b + i] = nodeWeight(node, i);
    }
  }
  nodeBegin_ = newBegin;

  // Assign pins
  for (Index hedge = 0; hedge < nHedges_; ++hedge) {
    for (Index node : hedgeNodes(hedge)) {
      Index ind = --newBegin[node+1];
      newData[ind] = hedge;
    }
  }

  nodeData_ = newData;
}

namespace {
class HedgeHasher {
 public:
  HedgeHasher(const Hypergraph &hypergraph)
    : hypergraph_(hypergraph) {}

  size_t operator()(const Index &hedge) const {
    // FNV hash
    uint64_t magic = 1099511628211llu;
    uint64_t ret = 0;
    for (Index n : hypergraph_.hedgeNodes(hedge)) {
      ret = (ret ^ (uint64_t) n) * magic;
    }
    return ret;
  }

 private:
  const Hypergraph &hypergraph_;
};

class HedgeComparer {
 public:
  HedgeComparer(const Hypergraph &hypergraph)
    : hypergraph_(hypergraph) {}

  bool operator()(const Index &h1, const Index &h2) const {
    return hypergraph_.hedgeNodes(h1) == hypergraph_.hedgeNodes(h2);
  }

 private:
  const Hypergraph &hypergraph_;
};
} // End anonymous namespace

void Hypergraph::mergeParallelHedges() {
  vector<Index> newHedgeBegin;
  vector<Index> newHedgeData;
  newHedgeBegin.push_back(0);

  unordered_map<Index, Index, HedgeHasher, HedgeComparer> hedgeWeightMap(nHedges(), HedgeHasher(*this), HedgeComparer(*this));
  for (Index hedge = 0; hedge < nHedges_; ++hedge) {
    Index ind = newHedgeBegin.size() - 1;
    auto p = hedgeWeightMap.emplace(hedge, ind);
    if (p.second) {
      // No such hyperedge exists yet
      for (Index i = 0; i < nHedgeWeights_; ++i) {
        newHedgeData.push_back(hedgeWeight(hedge, i));
      }
      for (Index node : hedgeNodes(hedge)) {
        newHedgeData.push_back(node);
      }
      newHedgeBegin.push_back(newHedgeData.size());
    }
    else {
      // An equivalent hyperedge exists already
      Index s = newHedgeBegin[p.first->second];
      for (Index i = 0; i < nHedgeWeights_; ++i) {
        newHedgeData[s + i] += hedgeWeight(hedge, i);
      }
    }
  }

  nHedges_ = newHedgeBegin.size() - 1;
  hedgeBegin_ = newHedgeBegin;
  hedgeData_ = newHedgeData;

  finalize();
}

void Hypergraph::setupBlocks(Index nParts, double imbalanceFactor) {
  // Setup partitions with weights proportional to the nodes
  if (nPartWeights_ != nNodeWeights_)
    throw runtime_error("Unable to generate partition capacities when the number of weights is not the same for parts and nodes");
  nParts_ = nParts;
  if (nParts > 0) {
    partData_.resize(nPartWeights_ * nParts);
    for (Index i = 0; i < nPartWeights_; ++i) {
      Index totalCapacity = totalNodeWeight(i) * (1.0 + imbalanceFactor);
      Index partitionCapacity = totalCapacity  / nParts;
      partData_[i] = totalCapacity - partitionCapacity * (nParts - 1);
      for (Index p = 1; p < nParts; ++p) {
        partData_[p * nPartWeights_ + i] = partitionCapacity;
      }
    }
  }
  else {
    partData_.clear();
  }
  finalizePartWeights();
}

bool Hypergraph::cut(const Solution &solution, Index hedge) const {
  unordered_set<Index> parts;
  for (Index node : hedgeNodes(hedge)) {
    parts.insert(solution[node]);
  }
  return parts.size() > 1;
}

Index Hypergraph::degree(const Solution &solution, Index hedge) const {
  unordered_set<Index> parts;
  for (Index node : hedgeNodes(hedge)) {
    parts.insert(solution[node]);
  }
  return parts.size();
}

} // End namespace minipart

