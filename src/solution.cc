// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "solution.hh"

#include <stdexcept>
#include <algorithm>

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

} // End namespace minipart

