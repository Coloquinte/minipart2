// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#ifndef MINIPART_INCREMENTAL_OBJECTIVE_HH
#define MINIPART_INCREMENTAL_OBJECTIVE_HH

#include "hypergraph.hh"

#include <memory>

namespace minipart {

/**
 * Base class for incremental evaluation
 */
class IncrementalObjective {
 public:
  IncrementalObjective(const Hypergraph &hypergraph, Solution &solution, Index nObjectives);

  virtual void move(Index node, Index to) =0;
  virtual void checkConsistency() const;

  Index nNodes() const { return hypergraph_.nNodes(); }
  Index nHedges() const { return hypergraph_.nHedges(); }
  Index nParts() const { return hypergraph_.nParts(); }
  Index nObjectives() const { return objectives_.size(); }

  const Hypergraph &hypergraph() const { return hypergraph_; }
  const Solution& solution() const { return solution_; }
  const std::vector<std::int64_t>& objectives() const { return objectives_; }

  static std::unique_ptr<IncrementalObjective> cut(const Hypergraph&, Solution&);
  static std::unique_ptr<IncrementalObjective> soed(const Hypergraph&, Solution&);
  static std::unique_ptr<IncrementalObjective> maxDegree(const Hypergraph&, Solution&);

 protected:
  const Hypergraph &hypergraph_;
  Solution &solution_;
  std::vector<std::int64_t> objectives_;
};

class IncrementalCut final : public IncrementalObjective {
 public:
  IncrementalCut(const Hypergraph &hypergraph, Solution &solution);
  void move(Index node, Index to) override;
  void checkConsistency() const override;

 private:
  std::vector<Index> partitionDemands_;
  std::vector<std::vector<Index> > hedgeNbPinsPerPartition_;
  std::vector<Index> hedgeDegrees_;
  Index currentCut_;
  Index currentSoed_;
};

class IncrementalSoed final : public IncrementalObjective {
 public:
  IncrementalSoed(const Hypergraph &hypergraph, Solution &solution);
  void move(Index node, Index to) override;
  void checkConsistency() const override;

 private:
  std::vector<Index> partitionDemands_;
  std::vector<std::vector<Index> > hedgeNbPinsPerPartition_;
  std::vector<Index> hedgeDegrees_;
  Index currentSoed_;
};

class IncrementalMaxDegree final : public IncrementalObjective {
 public:
  IncrementalMaxDegree(const Hypergraph &hypergraph, Solution &solution);
  void move(Index node, Index to) override;
  void checkConsistency() const override;

 private:
  std::vector<Index> partitionDemands_;
  std::vector<std::vector<Index> > hedgeNbPinsPerPartition_;
  std::vector<Index> hedgeDegrees_;
  std::vector<Index> partitionDegrees_;
  Index currentSoed_;
};

} // End namespace minipart

#endif
