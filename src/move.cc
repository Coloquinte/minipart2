
#include "move.hh"

#include <cassert>

using namespace std;

namespace minipart {
void SimpleMove::run(IncrementalObjective &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  std::uniform_int_distribution<Index> nodeDist(0, inc.nNodes()-1);
  std::uniform_int_distribution<Index> partDist(0, inc.nParts()-1);
  Index node = nodeDist(rgen);
  Index src = inc.solution()[node];
  Index dst = partDist(rgen);

  vector<int64_t> before = inc.objectives();
  inc.move(node, dst);
  vector<int64_t> after = inc.objectives();
  if (before < after) {
    inc.move(node, src);
  }
  --this->budget_;
}

void SimpleSwap::run(IncrementalObjective &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  --this->budget_;
  std::uniform_int_distribution<Index> nodeDist(0, inc.nNodes()-1);
  Index n1 = nodeDist(rgen);
  Index n2 = nodeDist(rgen);
  Index p1 = inc.solution()[n1];
  Index p2 = inc.solution()[n2];
  if (p1 == p2) return;

  vector<int64_t> before = inc.objectives();
  inc.move(n1, p2);
  inc.move(n2, p1);
  vector<int64_t> after = inc.objectives();
  if (before < after) {
    inc.move(n1, p1);
    inc.move(n2, p2);
  }
}

void EdgeMove::run(IncrementalObjective &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  initialStatus_.clear();

  std::uniform_int_distribution<Index> edgeDist(0, inc.nHedges()-1);
  std::uniform_int_distribution<Index> partDist(0, inc.nParts()-1);
  Index hedge = edgeDist(rgen);
  Index dst = partDist(rgen);
  if (inc.hypergraph().hedgeNodes(hedge).size() > edgeDegreeCutoff_) {
    --this->budget_;
    return;
  }

  vector<int64_t> before = inc.objectives();
  for (Index node : inc.hypergraph().hedgeNodes(hedge)) {
    Index src = inc.solution()[node];
    inc.move(node, dst);
    initialStatus_.emplace_back(node, src);
  }
  vector<int64_t> after = inc.objectives();
  if (before < after) {
    for (std::pair<Index, Index> status : initialStatus_) {
      inc.move(status.first, status.second);
    }
  }
  this->budget_ -= inc.hypergraph().hedgeNodes(hedge).size();
}

void AbsorptionMove::run(IncrementalObjective &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  std::uniform_int_distribution<Index> nodeDist(0, inc.nNodes()-1);
  std::uniform_int_distribution<Index> partDist(0, inc.nParts()-1);
  candidates_.clear();
  Index dst = partDist(rgen);
  candidates_.push_back(nodeDist(rgen));

  while (!candidates_.empty() && this->budget_ > 0) {
    Index node = candidates_.back();
    candidates_.pop_back();
    Index src = inc.solution()[node];
    if (src == dst)
      continue;
    --this->budget_;

    vector<int64_t> before = inc.objectives();
    inc.move(node, dst);
    vector<int64_t> after = inc.objectives();
    if (before < after) {
      inc.move(node, src);
    }
    else {
      if (inc.hypergraph().nodeHedges(node).size() <= nodeDegreeCutoff_) {
        for (Index hEdge : inc.hypergraph().nodeHedges(node)) {
          if (inc.hypergraph().hedgeNodes(hEdge).size() <= edgeDegreeCutoff_) {
            for (Index neighbour : inc.hypergraph().hedgeNodes(hEdge)) {
              candidates_.push_back(neighbour);
            }
          }
        }
      }
    }
  }
}

} // End namespace minipart

