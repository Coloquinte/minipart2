// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "incremental_objective.hh"

#include <cassert>
#include <algorithm>

using namespace std;

namespace minipart {

namespace {
vector<Index> computePartitionDemands(const Hypergraph &hypergraph, const Solution &solution) {
  vector<Index> ret(hypergraph.nParts(), 0);
  for (Index node = 0; node < hypergraph.nNodes(); ++node) {
    ret[solution[node]] += hypergraph.nodeWeight(node);
  }
  return ret;
}

vector<vector<Index> > computeHedgeNbPinsPerPartition(const Hypergraph &hypergraph, const Solution &solution) {
  vector<vector<Index> > ret(hypergraph.nHedges());
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    vector<Index> cnt(hypergraph.nParts());
    for (Index node : hypergraph.hedgeNodes(hedge)) {
      ++cnt[solution[node]];
    }
    ret[hedge] = cnt;
  }
  return ret;
}

vector<Index> computeHedgeDegrees(const Hypergraph &hypergraph, const vector<vector<Index> > &hedgeNbPinsPerPartition) {
  vector<Index> ret(hypergraph.nHedges());
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    Index degree = 0;
    for (Index cnt : hedgeNbPinsPerPartition[hedge]) {
      if (cnt != 0) ++degree;
    }
    ret[hedge] = degree;
  }
  return ret;
}

vector<Index> computePartitionDegrees(const Hypergraph &hypergraph, const vector<Index> &hedgeDegrees, const vector<vector<Index> > &hedgeNbPinsPerPartition) {
  vector<Index> ret(hypergraph.nParts(), 0);
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    if (hedgeDegrees[hedge] > 1) {
      for (Index p = 0; p < hypergraph.nParts(); ++p) {
        if (hedgeNbPinsPerPartition[hedge][p] != 0) {
          ret[p] += hypergraph.hedgeWeight(hedge);
        }
      }
    }
  }
  return ret;
}

Index computeDaisyChainDistance(const Hypergraph &hypergraph, const vector<vector<Index> > &hedgeNbPinsPerPartition) {
  Index distance = 0;
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    Index minPart = hypergraph.nParts() - 1;
    Index maxPart = 0;
    for (Index p = 0; p < hypergraph.nParts(); ++p) {
      if (hedgeNbPinsPerPartition[hedge][p] != 0) {
        minPart = min(minPart, p);
        maxPart = max(maxPart, p);
      }
    }
    distance += hypergraph.hedgeWeight(hedge) * (maxPart - minPart);
  }
  return distance;
}

vector<Index> computeDaisyChainPartitionDegrees(const Hypergraph &hypergraph, const vector<vector<Index> > &hedgeNbPinsPerPartition) {
  vector<Index> ret(hypergraph.nParts(), 0);
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    Index minPart = hypergraph.nParts() - 1;
    Index maxPart = 0;
    for (Index p = 0; p < hypergraph.nParts(); ++p) {
      if (hedgeNbPinsPerPartition[hedge][p] != 0) {
        minPart = min(minPart, p);
        maxPart = max(maxPart, p);
      }
    }
    for (Index p = minPart; p < maxPart; ++p) {
      ret[p] += hypergraph.hedgeWeight(hedge);
      ret[p+1] += hypergraph.hedgeWeight(hedge);
    }
  }
  return ret;
}


Index computeCut(const Hypergraph &hypergraph, const vector<Index> &hedgeDegrees) {
  Index ret = 0;
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    if (hedgeDegrees[hedge] > 1)
      ret += hypergraph.hedgeWeight(hedge);
  }
  return ret;
}

Index computeSoed(const Hypergraph &hypergraph, const vector<Index> &hedgeDegrees) {
  Index ret = 0;
  for (Index hedge = 0; hedge < hypergraph.nHedges(); ++hedge) {
    ret += hypergraph.hedgeWeight(hedge) * hedgeDegrees[hedge];
  }
  return ret;
}

Index computeSumOverflow(const Hypergraph &hypergraph, const vector<Index> &partitionDemands) {
  Index ret = 0;
  for (Index p = 0; p < hypergraph.nParts(); ++p) {
    ret += max(partitionDemands[p] - hypergraph.partWeight(p), (Index) 0);
  }
  return ret;
}

Index computeMaxDegree(const Hypergraph &, const vector<Index> &partitionDegrees) {
  return *max_element(partitionDegrees.begin(), partitionDegrees.end());
}
}

IncrementalObjective::IncrementalObjective(const Hypergraph &hypergraph, Solution &solution, Index nObjectives)
: hypergraph_(hypergraph)
, solution_(solution)
, objectives_(nObjectives, 0) {
  assert (hypergraph_.nNodes() == solution_.nNodes());
  assert (hypergraph_.nParts() == solution_.nParts());
}

void IncrementalObjective::checkConsistency() const {
}

IncrementalCut::IncrementalCut(const Hypergraph &hypergraph, Solution &solution)
: IncrementalObjective(hypergraph, solution, 3) {
  partitionDemands_ = computePartitionDemands(hypergraph, solution);
  hedgeNbPinsPerPartition_ = computeHedgeNbPinsPerPartition(hypergraph, solution);
  hedgeDegrees_ = computeHedgeDegrees(hypergraph, hedgeNbPinsPerPartition_);
  currentCut_ = computeCut(hypergraph, hedgeDegrees_);
  currentSoed_ = computeSoed(hypergraph, hedgeDegrees_);
}

IncrementalSoed::IncrementalSoed(const Hypergraph &hypergraph, Solution &solution)
: IncrementalObjective(hypergraph, solution, 2) {
  partitionDemands_ = computePartitionDemands(hypergraph, solution);
  hedgeNbPinsPerPartition_ = computeHedgeNbPinsPerPartition(hypergraph, solution);
  hedgeDegrees_ = computeHedgeDegrees(hypergraph, hedgeNbPinsPerPartition_);
  currentSoed_ = computeSoed(hypergraph, hedgeDegrees_);
}

IncrementalMaxDegree::IncrementalMaxDegree(const Hypergraph &hypergraph, Solution &solution)
: IncrementalObjective(hypergraph, solution, 3) {
  partitionDemands_ = computePartitionDemands(hypergraph, solution);
  hedgeNbPinsPerPartition_ = computeHedgeNbPinsPerPartition(hypergraph, solution);
  hedgeDegrees_ = computeHedgeDegrees(hypergraph, hedgeNbPinsPerPartition_);
  partitionDegrees_ = computePartitionDegrees(hypergraph, hedgeDegrees_, hedgeNbPinsPerPartition_);
  currentSoed_ = computeSoed(hypergraph, hedgeDegrees_);
}

IncrementalDaisyChainDistance::IncrementalDaisyChainDistance(const Hypergraph &hypergraph, Solution &solution)
: IncrementalObjective(hypergraph, solution, 3) {
  partitionDemands_ = computePartitionDemands(hypergraph, solution);
  hedgeNbPinsPerPartition_ = computeHedgeNbPinsPerPartition(hypergraph, solution);
  hedgeDegrees_ = computeHedgeDegrees(hypergraph, hedgeNbPinsPerPartition_);
  currentDistance_ = computeDaisyChainDistance(hypergraph, hedgeNbPinsPerPartition_);
  currentSoed_ = computeSoed(hypergraph, hedgeDegrees_);
}

IncrementalDaisyChainMaxDegree::IncrementalDaisyChainMaxDegree(const Hypergraph &hypergraph, Solution &solution)
: IncrementalObjective(hypergraph, solution, 3) {
  partitionDemands_ = computePartitionDemands(hypergraph, solution);
  hedgeNbPinsPerPartition_ = computeHedgeNbPinsPerPartition(hypergraph, solution);
  hedgeDegrees_ = computeHedgeDegrees(hypergraph, hedgeNbPinsPerPartition_);
  partitionDegrees_ = computeDaisyChainPartitionDegrees(hypergraph, hedgeNbPinsPerPartition_);
  currentDistance_ = computeDaisyChainDistance(hypergraph, hedgeNbPinsPerPartition_);
}

void IncrementalCut::checkConsistency() const {
  assert (partitionDemands_ == computePartitionDemands(hypergraph_, solution_));
  assert (hedgeNbPinsPerPartition_ == computeHedgeNbPinsPerPartition(hypergraph_, solution_));
  assert (hedgeDegrees_ == computeHedgeDegrees(hypergraph_, hedgeNbPinsPerPartition_));
  assert (currentCut_ == computeCut(hypergraph_, hedgeDegrees_));
  assert (currentSoed_ == computeSoed(hypergraph_, hedgeDegrees_));
}

void IncrementalSoed::checkConsistency() const {
  assert (partitionDemands_ == computePartitionDemands(hypergraph_, solution_));
  assert (hedgeNbPinsPerPartition_ == computeHedgeNbPinsPerPartition(hypergraph_, solution_));
  assert (hedgeDegrees_ == computeHedgeDegrees(hypergraph_, hedgeNbPinsPerPartition_));
  assert (currentSoed_ == computeSoed(hypergraph_, hedgeDegrees_));
}

void IncrementalMaxDegree::checkConsistency() const {
  assert (partitionDemands_ == computePartitionDemands(hypergraph_, solution_));
  assert (hedgeNbPinsPerPartition_ == computeHedgeNbPinsPerPartition(hypergraph_, solution_));
  assert (hedgeDegrees_ == computeHedgeDegrees(hypergraph_, hedgeNbPinsPerPartition_));
  assert (partitionDegrees_ == computePartitionDegrees(hypergraph_, hedgeDegrees_, hedgeNbPinsPerPartition_));
  assert (currentSoed_ == computeSoed(hypergraph_, hedgeDegrees_));
}

void IncrementalDaisyChainDistance::checkConsistency() const {
  assert (partitionDemands_ == computePartitionDemands(hypergraph_, solution_));
  assert (hedgeNbPinsPerPartition_ == computeHedgeNbPinsPerPartition(hypergraph_, solution_));
  assert (hedgeDegrees_ == computeHedgeDegrees(hypergraph_, hedgeNbPinsPerPartition_));
  assert (currentDistance_ == computeDaisyChainDistance(hypergraph_, hedgeNbPinsPerPartition_));
  assert (currentSoed_ == computeSoed(hypergraph_, hedgeDegrees_));
}

void IncrementalDaisyChainMaxDegree::checkConsistency() const {
  assert (partitionDemands_ == computePartitionDemands(hypergraph_, solution_));
  assert (hedgeNbPinsPerPartition_ == computeHedgeNbPinsPerPartition(hypergraph_, solution_));
  assert (hedgeDegrees_ == computeHedgeDegrees(hypergraph_, hedgeNbPinsPerPartition_));
  assert (currentDistance_ == computeDaisyChainDistance(hypergraph_, hedgeNbPinsPerPartition_));
  assert (partitionDegrees_ == computeDaisyChainPartitionDegrees(hypergraph_, hedgeNbPinsPerPartition_));
}

void IncrementalCut::move(Index node, Index to) {
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
    if (pins[to] == 1 && pins[from] != 0) {
      ++hedgeDegrees_[hedge];
      if (hedgeDegrees_[hedge] == 2) {
        currentCut_ += hypergraph_.hedgeWeight(hedge);
      }
      currentSoed_ += hypergraph_.hedgeWeight(hedge);
    }
    if (pins[to] != 1 && pins[from] == 0) {
      --hedgeDegrees_[hedge];
      if (hedgeDegrees_[hedge] == 1) {
        currentCut_ -= hypergraph_.hedgeWeight(hedge);
      }
      currentSoed_ -= hypergraph_.hedgeWeight(hedge);
    }
  }
  objectives_[0] = computeSumOverflow(hypergraph_, partitionDemands_);
  objectives_[1] = currentCut_;
  objectives_[2] = currentSoed_;
}

void IncrementalSoed::move(Index node, Index to) {
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
    if (pins[to] == 1 && pins[from] != 0) {
      ++hedgeDegrees_[hedge];
      currentSoed_ += hypergraph_.hedgeWeight(hedge);
    }
    if (pins[to] != 1 && pins[from] == 0) {
      --hedgeDegrees_[hedge];
      currentSoed_ -= hypergraph_.hedgeWeight(hedge);
    }
  }
  objectives_[0] = computeSumOverflow(hypergraph_, partitionDemands_);
  objectives_[1] = currentSoed_;
}

void IncrementalMaxDegree::move(Index node, Index to) {
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
        becomesCut = true;
      }
      currentSoed_ += hypergraph_.hedgeWeight(hedge);
    }
    if (pins[to] != 1 && pins[from] == 0) {
      --hedgeDegrees_[hedge];
      if (hedgeDegrees_[hedge] == 1) {
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

  objectives_[0] = computeSumOverflow(hypergraph_, partitionDemands_);
  objectives_[1] = computeMaxDegree(hypergraph_, partitionDegrees_);
  objectives_[2] = currentSoed_;
}

void IncrementalDaisyChainDistance::move(Index node, Index to) {
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
    bool reachesPart = pins[to] == 1;
    bool leavesPart = pins[from] == 0;
    if (reachesPart) {
      ++hedgeDegrees_[hedge];
      currentSoed_ += hypergraph_.hedgeWeight(hedge);
    }
    if (leavesPart) {
      --hedgeDegrees_[hedge];
      currentSoed_ -= hypergraph_.hedgeWeight(hedge);
    }
    if (reachesPart || leavesPart) {
      Index minBefore = nParts() - 1;
      Index maxBefore = 0;
      Index minAfter = nParts() - 1;
      Index maxAfter = 0;
      for (Index p = 0; p < nParts(); ++p) {
        bool countsBefore = p == to ? !reachesPart : pins[p] != 0 || p == from;
        bool countsAfter = pins[p] != 0;
        if (countsBefore) {
          minBefore = min(minBefore, p);
          maxBefore = max(maxBefore, p);
        }
        if (countsAfter) {
          minAfter = min(minAfter, p);
          maxAfter = max(maxAfter, p);
        }
      }
      currentDistance_ += hypergraph_.hedgeWeight(hedge) * (maxAfter - minAfter - maxBefore + minBefore);
    }
  }

  objectives_[0] = computeSumOverflow(hypergraph_, partitionDemands_);
  objectives_[1] = currentDistance_;
  objectives_[2] = currentSoed_;
}

void IncrementalDaisyChainMaxDegree::move(Index node, Index to) {
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
    bool reachesPart = pins[to] == 1;
    bool leavesPart = pins[from] == 0;
    if (reachesPart) {
      ++hedgeDegrees_[hedge];
    }
    if (leavesPart) {
      --hedgeDegrees_[hedge];
    }
    if (reachesPart || leavesPart) {
      Index minBefore = nParts() - 1;
      Index maxBefore = 0;
      Index minAfter = nParts() - 1;
      Index maxAfter = 0;
      for (Index p = 0; p < nParts(); ++p) {
        bool countsBefore = p == to ? !reachesPart : pins[p] != 0 || p == from;
        bool countsAfter = pins[p] != 0;
        if (countsBefore) {
          minBefore = min(minBefore, p);
          maxBefore = max(maxBefore, p);
        }
        if (countsAfter) {
          minAfter = min(minAfter, p);
          maxAfter = max(maxAfter, p);
        }
      }
      if (minAfter != minBefore || maxAfter != maxBefore) {
        currentDistance_ += hypergraph_.hedgeWeight(hedge) * (maxAfter - minAfter - maxBefore + minBefore);
        // Update degrees
        for (Index p = minBefore; p < maxBefore; ++p) {
          partitionDegrees_[p] -= hypergraph_.hedgeWeight(hedge);
          partitionDegrees_[p+1] -= hypergraph_.hedgeWeight(hedge);
        }
        for (Index p = minAfter; p < maxAfter; ++p) {
          partitionDegrees_[p] += hypergraph_.hedgeWeight(hedge);
          partitionDegrees_[p+1] += hypergraph_.hedgeWeight(hedge);
        }
      }
    }
  }

  objectives_[0] = computeSumOverflow(hypergraph_, partitionDemands_);
  objectives_[1] = computeMaxDegree(hypergraph_, partitionDegrees_);
  objectives_[2] = currentDistance_;
}

} // End namespace minipart
