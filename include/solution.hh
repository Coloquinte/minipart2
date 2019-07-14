// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_SOLUTION_HH
#define MINIPART_SOLUTION_HH

#include "common.hh"
#include <iosfwd>

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

  void resizeParts(Index parts);

  // IO functions
  static Solution readFile(const std::string &name);
  void writeFile(const std::string &name) const;
  static Solution read(std::istream &);
  void write(std::ostream &) const;

  void checkConsistency() const;

 private:
  std::vector<Index> parts_;
  Index nParts_;
};

} // End namespace minipart

#endif

