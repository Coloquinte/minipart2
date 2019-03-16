// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "hypergraph.hh"

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>

using namespace std;

namespace minipart {

Hypergraph Hypergraph::coarsen(const Solution &coarsening) const {
  assert (nNodes() == coarsening.nNodes());
  assert (coarsening.nParts() <= nNodes());
  if (nNodes() == 0) return Hypergraph();

  Hypergraph ret;

  // Node weights
  ret.nodeWeights_.resize(coarsening.nParts(), 0);
  for (Index node = 0; node < nNodes(); ++node) {
    ret.nodeWeights_[coarsening[node]] += nodeWeights_[node];
  }

  // Hyperedges
  vector<Index> pins;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    for (Index node : hedgeNodes(hedge)) {
      pins.push_back(coarsening[node]);
    }
    sort(pins.begin(), pins.end());
    pins.resize(unique(pins.begin(), pins.end()) - pins.begin());
    if (pins.size() > 1) {
      ret.hedgeWeights_.push_back(hedgeWeights_[hedge]);
      ret.hedgeToNodes_.push_back(pins);
    }
    pins.clear();
  }
  ret.mergeParallelHedges();

  // Partitions
  ret.partWeights_ = partWeights_;

  return ret;
}

void Hypergraph::checkConsistency() const {
  if (nodeWeights_.size() != nodeToHedges_.size())
    throw runtime_error("Number of node weights and of nodes do not match");
  if (hedgeWeights_.size() != hedgeToNodes_.size())
    throw runtime_error("Number of hedge weights and of hedges do not match");

  for (const vector<Index> &node : nodeToHedges_) {
    for (Index hedge : node) {
      if (hedge < 0 || hedge >= nHedges())
        throw runtime_error("Invalid hedge value");
    }
    unordered_set<Index> uq(node.begin(), node.end());
    if (uq.size() != node.size())
        throw runtime_error("Duplicate hedges in a node");
  }
  for (const vector<Index> &hedge : hedgeToNodes_) {
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

Index Hypergraph::nPins() const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeNodes(hedge).size();
  }
  return ret;
}

Index Hypergraph::totalHedgeWeight() const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge);
  }
  return ret;
}

Index Hypergraph::totalNodeWeight() const {
  Index ret = 0;
  for (Index node = 0; node < nNodes(); ++node) {
    ret += nodeWeight(node);
  }
  return ret;
}

void Hypergraph::constructNodes() {
  nodeToHedges_.clear();
  nodeToHedges_.resize(nodeWeights_.size());
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    for (Index node : hedgeNodes(hedge)) {
      nodeToHedges_[node].push_back(hedge);
    }
  }
}

void Hypergraph::constructHedges() {
  hedgeToNodes_.clear();
  hedgeToNodes_.resize(hedgeWeights_.size());
  for (Index node = 0; node < nHedges(); ++node) {
    for (Index hedge : nodeHedges(node)) {
      hedgeToNodes_[hedge].push_back(node);
    }
  }
}

Index Hypergraph::metricsCut(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    if (cut(solution, hedge))
      ret += hedgeWeight(hedge);
  }
  return ret;
}

Index Hypergraph::metricsSoed(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge) * degree(solution, hedge);
  }
  return ret;
}

Index Hypergraph::metricsConnectivity(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge) * (degree(solution, hedge) - 1);
  }
  return ret;
}

Index Hypergraph::metricsSumOverflow(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  vector<Index> usage(nParts(), 0);
  for (int i = 0; i < nNodes(); ++i) {
    assert (solution[i] >= 0 && solution[i] < nParts());
    usage[solution[i]] += nodeWeight(i);
  }
  Index ret = 0;
  for (int i = 0; i < nParts(); ++i) {
    Index ovf = usage[i] - partWeights_[i];
    if (ovf > 0)
      ret += ovf;
  }
  return ret;
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
  vector<vector<Index> > newHedges;
  vector<Index> newWeights;

  unordered_map<Index, Index, HedgeHasher, HedgeComparer> hedgeWeightMap(nHedges(), HedgeHasher(*this), HedgeComparer(*this));
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    Index ind = newHedges.size();
    auto p = hedgeWeightMap.emplace(hedge, ind);
    if (p.second) {
      newHedges.push_back(hedgeNodes(hedge));
      newWeights.push_back(hedgeWeight(hedge));
    }
    else {
      // An equivalent hyperedge exists already
      newWeights[p.first->second] += hedgeWeight(hedge);
    }
  }

  hedgeToNodes_ = newHedges;
  hedgeWeights_ = newWeights;

  constructNodes();
}

void Hypergraph::setupPartitions(Index nParts, double imbalanceFactor) {
  if (nParts > 0) {
    Index totalCapacity = totalNodeWeight() * (1.0 + imbalanceFactor);
    Index partitionCapacity = totalCapacity  / nParts;
    partWeights_.assign(nParts, partitionCapacity);
    partWeights_[0] = totalCapacity - partitionCapacity * (nParts - 1);
  }
  else {
    partWeights_.clear();
  }
}

} // End namespace minipart

