// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "incremental_solution.hh"

#include <cassert>
#include <algorithm>

using namespace std;

namespace minipart {

IncrementalSolution::IncrementalSolution(const Hypergraph &hypergraph, Solution &solution)
: hypergraph_(hypergraph)
, solution_(solution) {
  assert (hypergraph_.nNodes() == solution_.nNodes());
  assert (hypergraph_.nParts() == solution_.nParts());
  partitionDemands_ = computePartitionDemands();
  hedgeNbPinsPerPartition_ = computeHedgeNbPinsPerPartition();
  hedgeDegrees_ = computeHedgeDegrees();
  partitionDegrees_ = computePartitionDegrees();
  currentCut_ = computeCut();
  currentSoed_ = computeSoed();
}

void IncrementalSolution::checkConsistency() const {
  assert (hypergraph_.nNodes() == solution_.nNodes());
  assert (hypergraph_.nParts() == solution_.nParts());
  assert (partitionDemands_ == computePartitionDemands());
  assert (hedgeNbPinsPerPartition_ == computeHedgeNbPinsPerPartition());
  assert (hedgeDegrees_ == computeHedgeDegrees());
  assert (partitionDegrees_ == computePartitionDegrees());
  assert (currentCut_ == computeCut());
  assert (currentSoed_ == computeSoed());
}

vector<Index> IncrementalSolution::computePartitionDemands() const {
  vector<Index> ret(nParts(), 0);
  for (Index node = 0; node < nNodes(); ++node) {
    ret[solution_[node]] += hypergraph_.nodeWeight(node);
  }
  return ret;
}

vector<vector<Index> > IncrementalSolution::computeHedgeNbPinsPerPartition() const {
  vector<vector<Index> > ret(nHedges());
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    vector<Index> cnt(nParts());
    for (Index node : hypergraph_.hedgeNodes(hedge)) {
      ++cnt[solution_[node]];
    }
    ret[hedge] = cnt;
  }
  return ret;
}

vector<Index> IncrementalSolution::computeHedgeDegrees() const {
  vector<Index> ret(nHedges());
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    Index degree = 0;
    for (Index cnt : hedgeNbPinsPerPartition_[hedge]) {
      if (cnt != 0) ++degree;
    }
    ret[hedge] = degree;
  }
  return ret;
}

vector<Index> IncrementalSolution::computePartitionDegrees() const {
  vector<Index> ret(nParts(), 0);
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    if (!cut(hedge)) continue;
    for (Index p = 0; p < nParts(); ++p) {
      if (hedgeNbPinsPerPartition_[hedge][p] != 0) {
        ret[p] += hypergraph_.hedgeWeight(hedge);
      }
    }
  }
  return ret;
}

Index IncrementalSolution::computeCut() const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    if (hedgeDegrees_[hedge] > 1)
      ret += hypergraph_.hedgeWeight(hedge);
  }
  return ret;
}

Index IncrementalSolution::computeSoed() const {
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hypergraph_.hedgeWeight(hedge) * hedgeDegrees_[hedge];
  }
  return ret;
}

Index IncrementalSolution::metricsSumOverflow() const {
  Index ret = 0;
  for (Index p = 0; p < nParts(); ++p) {
    ret += max(demand(p) - capacity(p), (Index) 0);
  }
  return ret;
}

Index IncrementalSolution::metricsMaxDegree() const {
  return *max_element(partitionDegrees_.begin(), partitionDegrees_.end());
}

void IncrementalSolution::move(Index node, Index to) {
  assert (to < nParts() && to >= 0);
  Index from = solution_[node];
  if (from == to) return;
  solution_[node] = to;
  partitionDemands_[to]   += hypergraph_.nodeWeight(node);
  partitionDemands_[from] -= hypergraph_.nodeWeight(node);

  for (Index hedge : hypergraph_.nodeHedges(node)) {
    vector<Index> &pins = hedgeNbPinsPerPartition_[hedge];
    ++pins[to];
    --pins[from];
    bool becomesCut = false;
    bool becomesUncut = false;
    if (pins[to] == 1 && pins[from] != 0) {
      ++hedgeDegrees_[hedge];
      if (hedgeDegrees_[hedge] == 2) {
        currentCut_ += hypergraph_.hedgeWeight(hedge);
        becomesCut = true;
      }
      currentSoed_ += hypergraph_.hedgeWeight(hedge);
    }
    if (pins[to] != 1 && pins[from] == 0) {
      --hedgeDegrees_[hedge];
      if (hedgeDegrees_[hedge] == 1) {
        currentCut_ -= hypergraph_.hedgeWeight(hedge);
        becomesUncut = true;
      }
      currentSoed_ -= hypergraph_.hedgeWeight(hedge);
    }

    if (becomesUncut) {
      partitionDegrees_[from] -= hypergraph_.hedgeWeight(hedge);
      partitionDegrees_[to] -= hypergraph_.hedgeWeight(hedge);
    }
    else if (becomesCut) {
      partitionDegrees_[from] += hypergraph_.hedgeWeight(hedge);
      partitionDegrees_[to] += hypergraph_.hedgeWeight(hedge);
    }
    else if (hedgeDegrees_[hedge] >= 2) {
      if (pins[from] == 0) {
        partitionDegrees_[from] -= hypergraph_.hedgeWeight(hedge);
      }
      if (pins[to] == 1) {
        partitionDegrees_[to] += hypergraph_.hedgeWeight(hedge);
      }
    }
  }
}

} // End namespace minipart
