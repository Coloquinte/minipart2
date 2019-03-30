
#ifndef MINIPART_GENERIC_LOCAL_SEARCH_HH
#define MINIPART_GENERIC_LOCAL_SEARCH_HH

#include "local_search.hh"
#include "incremental_solution.hh"
#include "partitioning_params.hh"

#include <random>
#include <iosfwd>

namespace minipart {

template<typename TObjectiveType>
class GenericLocalSearch : public LocalSearch {
 public:
  void run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const final override;
  bool compare(const Hypergraph &hypergraph, const Solution &sol1, const Solution &sol2) const final override;
  void report(const Hypergraph &hypergraph, const Solution &solution, std::ostream &os) const final override;
 
 private:
  class Runner {
   public:
    Runner(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen);
    void simpleMoves(Index nMoves);
    void simpleSwaps(Index nMoves);
    void absorptionMoves(Index nMoves);

   private:
    IncrementalSolution inc_;
    std::mt19937 &rgen_;
    std::vector<Index> candidates_;
  };
};

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const {
  Index nMoves = params.movesPerElement * hypergraph.nNodes() * (hypergraph.nParts()-1);
  Runner runner(hypergraph, solution, params, rgen);
  runner.simpleSwaps(nMoves);
  runner.absorptionMoves(nMoves);
}

template<typename TObjectiveType>
inline bool GenericLocalSearch<TObjectiveType>::compare(const Hypergraph &hypergraph, const Solution &sol1, const Solution &sol2) const {
  return TObjectiveType(hypergraph, sol1) < TObjectiveType(hypergraph, sol2);
}

template<typename TObjectiveType>
inline GenericLocalSearch<TObjectiveType>::Runner::Runner(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen)
: inc_(hypergraph, solution)
, rgen_(rgen) {
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::Runner::simpleMoves(Index nMoves) {
  std::uniform_int_distribution<int> partDist(0, inc_.nParts()-1);
  std::uniform_int_distribution<int> nodeDist(0, inc_.nNodes()-1);
  for (int iter = 0; iter < nMoves; ++iter) {
    Index node = nodeDist(rgen_);
    Index src = inc_.solution(node);
    Index dst = partDist(rgen_);

    TObjectiveType before(inc_);
    inc_.move(node, dst);
    TObjectiveType after(inc_);
    if (before < after) {
      inc_.move(node, src);
    }
  }
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::Runner::simpleSwaps(Index nMoves) {
  std::uniform_int_distribution<int> nodeDist(0, inc_.nNodes()-1);
  for (int iter = 0; iter < nMoves; ++iter) {
    Index n1 = nodeDist(rgen_);
    Index n2 = nodeDist(rgen_);
    Index p1 = inc_.solution(n1);
    Index p2 = inc_.solution(n1);
    if (p1 == p2) continue;

    TObjectiveType before(inc_);
    inc_.move(n1, p2);
    inc_.move(n2, p1);
    TObjectiveType after(inc_);
    if (before < after) {
      inc_.move(n1, p1);
      inc_.move(n2, p2);
    }
  }
}

template<typename TObjectiveType>
inline void GenericLocalSearch<TObjectiveType>::Runner::absorptionMoves(Index nMoves) {
  std::uniform_int_distribution<int> partDist(0, inc_.nParts()-1);
  std::uniform_int_distribution<int> nodeDist(0, inc_.nNodes()-1);
  inc_.hypergraph().checkConsistency();
  candidates_.clear();
  for (int iter = 0; iter < nMoves;) {
    Index dst = partDist(rgen_);
    candidates_.push_back(nodeDist(rgen_));

    while (!candidates_.empty() && iter < nMoves) {
      Index node = candidates_.back();
      candidates_.pop_back();
      Index src = inc_.solution(node);
      if (src == dst)
        continue;

      TObjectiveType before(inc_);
      inc_.move(node, dst);
      TObjectiveType after(inc_);
      if (before < after) {
        inc_.move(node, src);
      }
      else {
        for (Index hEdge : inc_.hypergraph().nodeHedges(node)) {
          // TODO: try better rules that do not involve all pins or all partitions if possible
          for (Index neighbour : inc_.hypergraph().hedgeNodes(hEdge)) {
            candidates_.push_back(neighbour);
          }
        }
      }
      ++iter;
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

