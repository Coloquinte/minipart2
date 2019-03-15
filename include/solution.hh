// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_SOLUTION_HH
#define MINIPART_SOLUTION_HH

#include "common.hh"

namespace minipart {

class Solution {
 public:
  Solution(Index nNodes, Index nParts);
  Solution(std::vector<Index> parts);

  Index nNodes() const { return parts_.size(); }
  Index nParts() const { return nParts_; }

  Index  operator[](Index node) const { return parts_[node]; }
  Index& operator[](Index node) { return parts_[node]; }

  Solution coarsen(const Solution &coarsening) const;
  Solution uncoarsen(const Solution &coarsening) const;

  void checkConsistency() const;

 private:
  std::vector<Index> parts_;
  Index nParts_;
};

} // End namespace minipart

#endif

