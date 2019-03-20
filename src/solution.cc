// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "solution.hh"

#include <stdexcept>
#include <algorithm>
#include <cassert>

using namespace std;

namespace minipart {

Solution::Solution(Index nNodes, Index nParts)
: parts_(nNodes, 0)
, nParts_(nParts)
{
}

Solution::Solution(std::vector<Index> parts) {
  parts_ = move(parts);
  nParts_ = *max_element(parts_.begin(), parts_.end()) + 1;
}

void Solution::checkConsistency() const {
  for (Index p : parts_) {
    if (p < 0)
      throw runtime_error("Partition numbers must be non-negative");
    if (p >= nParts())
      throw runtime_error("Partition numbers must be smaller than the number of partitions");
  }
}

Solution Solution::coarsen(const Solution &coarsening) const {
  assert (coarsening.nNodes() == nNodes());
  Solution ret(coarsening.nParts(), nParts());
  for (Index node = 0; node < nNodes(); ++node) {
    ret[coarsening[node]] = (*this)[node];
  }
  return ret;
}

Solution Solution::uncoarsen(const Solution &coarsening) const {
  assert (coarsening.nParts() == nNodes());
  Solution ret(coarsening.nNodes(), nParts());
  for (Index node = 0; node < coarsening.nNodes(); ++node) {
    ret[node] = (*this)[coarsening[node]];
  }
  return ret;
}

} // End namespace minipart
