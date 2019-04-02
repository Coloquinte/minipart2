
#ifndef MINIPART_GENERIC_LOCAL_SEARCH_HH
#define MINIPART_GENERIC_LOCAL_SEARCH_HH

#include "local_search.hh"
#include "incremental_solution.hh"
#include "partitioning_params.hh"

#include <random>
#include <iosfwd>
#include <cassert>

namespace minipart {

template<typename TObjectiveType>
class GenericLocalSearch : public LocalSearch {
 public:
  void run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const final override;
  bool compare(const Hypergraph &hypergraph, const Solution &sol1, const Solution &sol2) const final override;
  void report(const Hypergraph &hypergraph, const Solution &solution, std::ostream &os) const final override;
 
 private:
  class Move {
   public:
    Move(Index budget) : budget_(budget) {}
    virtual void run(IncrementalSolution &inc, std::mt19937 &rgen) =0;
    virtual ~Move() {}

    std::uint64_t budget_;
  };

  class Runner {
   public:
    Runner(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen);
    void run();

   private:
    void init();
    void doMove();
    std::size_t totalBudget() const;

   private:
    IncrementalSolution inc_;
    const PartitioningParams &params_;
    std::mt19937 &rgen_;
    std::vector<std::unique_ptr<Move> > moves_;
  };

  class SimpleMove : public Move {
   public:
    SimpleMove(Index budget) : Move(budget) {}
    void run(IncrementalSolution &inc, std::mt19937 &rgen) override;
  };

  class SimpleSwap : public Move {
   public:
    SimpleSwap(Index budget) : Move(budget) {}
    void run(IncrementalSolution &inc, std::mt19937 &rgen) override;
  };

  class AbsorptionMove : public Move {
   public:
    AbsorptionMove(Index budget) : Move(budget) {}
    void run(IncrementalSolution &inc, std::mt19937 &rgen) override;

   private:
    std::vector<Index> candidates_;
  };
};

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const {
  Runner runner(hypergraph, solution, params, rgen);
  runner.run();
}

template<typename TObjectiveType>
inline bool GenericLocalSearch<TObjectiveType>::compare(const Hypergraph &hypergraph, const Solution &sol1, const Solution &sol2) const {
  return TObjectiveType(hypergraph, sol1) < TObjectiveType(hypergraph, sol2);
}

template<typename TObjectiveType>
inline GenericLocalSearch<TObjectiveType>::Runner::Runner(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen)
: inc_(hypergraph, solution)
, params_(params)
, rgen_(rgen) {
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::Runner::run() {
  init();
  while (totalBudget() > 0) {
    doMove();
  }
}

template<typename TObjectiveType>
inline std::size_t GenericLocalSearch<TObjectiveType>::Runner::totalBudget() const {
  std::size_t ret = 0;
  for (const std::unique_ptr<Move> &mv : moves_) {
    ret += mv->budget_;
  }
  return ret;
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::Runner::init() {
  moves_.clear();
  std::size_t nMoves = params_.movesPerElement * (std::size_t) inc_.nNodes() * (std::size_t) (inc_.nParts()-1);
  moves_.emplace_back(std::make_unique<SimpleMove>(nMoves));
  moves_.emplace_back(std::make_unique<SimpleSwap>(nMoves));
  moves_.emplace_back(std::make_unique<AbsorptionMove>(nMoves));
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::Runner::doMove() {
  // Pick a move based on the remaining budget
  std::uniform_int_distribution<std::size_t> dist(0, totalBudget() - 1);
  std::size_t roll = dist(rgen_);
  std::size_t tot = 0;
  for (std::unique_ptr<Move> &mv : moves_) {
    tot += mv->budget_;
    if (tot > roll) {
      mv->run(inc_, rgen_);
      return;
    }
  }
  assert (false);
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::SimpleMove::run(IncrementalSolution &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  std::uniform_int_distribution<Index> nodeDist(0, inc.nNodes()-1);
  std::uniform_int_distribution<Index> partDist(0, inc.nParts()-1);
  Index node = nodeDist(rgen);
  Index src = inc.solution(node);
  Index dst = partDist(rgen);

  TObjectiveType before(inc);
  inc.move(node, dst);
  TObjectiveType after(inc);
  if (before < after) {
    inc.move(node, src);
  }
  --this->budget_;
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::SimpleSwap::run(IncrementalSolution &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  --this->budget_;
  std::uniform_int_distribution<Index> nodeDist(0, inc.nNodes()-1);
  Index n1 = nodeDist(rgen);
  Index n2 = nodeDist(rgen);
  Index p1 = inc.solution(n1);
  Index p2 = inc.solution(n1);
  if (p1 == p2) return;

  TObjectiveType before(inc);
  inc.move(n1, p2);
  inc.move(n2, p1);
  TObjectiveType after(inc);
  if (before < after) {
    inc.move(n1, p1);
    inc.move(n2, p2);
  }
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::AbsorptionMove::run(IncrementalSolution &inc, std::mt19937 &rgen) {
  assert (this->budget_ > 0);
  std::uniform_int_distribution<Index> nodeDist(0, inc.nNodes()-1);
  std::uniform_int_distribution<Index> partDist(0, inc.nParts()-1);
  candidates_.clear();
  Index dst = partDist(rgen);
  candidates_.push_back(nodeDist(rgen));

  while (!candidates_.empty() && this->budget_ > 0) {
    Index node = candidates_.back();
    candidates_.pop_back();
    Index src = inc.solution(node);
    if (src == dst)
      continue;
    --this->budget_;

    TObjectiveType before(inc);
    inc.move(node, dst);
    TObjectiveType after(inc);
    if (before < after) {
      inc.move(node, src);
    }
    else {
      for (Index hEdge : inc.hypergraph().nodeHedges(node)) {
        // TODO: try better rules that do not involve all pins or all partitions if possible
        for (Index neighbour : inc.hypergraph().hedgeNodes(hEdge)) {
          candidates_.push_back(neighbour);
        }
      }
    }
  }
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::report(const Hypergraph &hypergraph, const Solution &solution, std::ostream &os) const {
  TObjectiveType f(hypergraph, solution);
  f.report(os);
}

} // End namespace minipart

#endif

