// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_OBJECTIVE_HH
#define MINIPART_OBJECTIVE_HH

#include "hypergraph.hh"
#include <memory>

namespace minipart {

/**
 * Base class for objectives
 */
class Objective {
 public:
  virtual std::unique_ptr<IncrementalObjective> incremental(const Hypergraph &, Solution &) =0;
  virtual ~Objective() {}
};

class CutObjective final : public Objective {
 public:
  std::unique_ptr<IncrementalObjective> incremental(const Hypergraph &, Solution &) override;
};

class SoedObjective final : public Objective {
 public:
  std::unique_ptr<IncrementalObjective> incremental(const Hypergraph &, Solution &) override;
};

class MaxDegreeObjective final : public Objective {
 public:
  std::unique_ptr<IncrementalObjective> incremental(const Hypergraph &, Solution &) override;
};

} // End namespace minipart

#endif

