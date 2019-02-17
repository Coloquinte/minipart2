
#include "hypergraph.hh"

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace minipart {

Hypergraph Hypergraph::coarsen(const vector<Index> &coarsening) const {
  assert (nNodes() == (Index) coarsening.size());
  if (nNodes() == 0) return Hypergraph();

  Index coarsenedNodes = *max_element(coarsening.begin(), coarsening.end()) + 1;
  assert (coarsenedNodes <= nNodes());

  Hypergraph ret;

  // Node weights
  ret.nodeWeights_.resize(coarsenedNodes);
  for (Index node = 0; node < nHedges(); ++node) {
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

  ret.constructNodes();

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
  }
  for (const vector<Index> &hedge : hedgeToNodes_) {
    for (Index node : hedge) {
      if (node < 0 || node >= nNodes())
        throw runtime_error("Invalid node value");
    }
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

void Hypergraph::constructNodes() {
  nodeToHedges_.resize(nodeWeights_.size());
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    for (Index node : hedgeNodes(hedge)) {
      nodeToHedges_[node].push_back(hedge);
    }
  }
}

void Hypergraph::constructHedges() {
  hedgeToNodes_.resize(hedgeWeights_.size());
  for (Index node = 0; node < nHedges(); ++node) {
    for (Index hedge : nodeHedges(node)) {
      hedgeToNodes_[hedge].push_back(node);
    }
  }
}

Index Hypergraph::metricsCut(const vector<Index> &solution) const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    if (cut(solution, hedge))
      ret += hedgeWeight(hedge);
  }
  return ret;
}

Index Hypergraph::metricsSoed(const vector<Index> &solution) const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge) * degree(solution, hedge);
  }
  return ret;
}

Index Hypergraph::metricsConnectivity(const vector<Index> &solution) const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge) * (degree(solution, hedge) - 1);
  }
  return ret;
}

Index Hypergraph::metricsModularity(const vector<Index> &solution) const {
  Index ret = 0;
  return ret;
}

bool Hypergraph::cut(const vector<Index> &solution, Index hedge) const {
  unordered_set<Index> parts;
  for (Index node : hedgeNodes(hedge)) {
    parts.insert(solution[node]);
  }
  return parts.size() > 1;
}

Index Hypergraph::degree(const vector<Index> &solution, Index hedge) const {
  unordered_set<Index> parts;
  for (Index node : hedgeNodes(hedge)) {
    parts.insert(solution[node]);
  }
  return parts.size();
}

} // End namespace minipart

