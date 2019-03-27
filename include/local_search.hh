
#ifndef MINIPART_LOCAL_SEARCH_HH
#define MINIPART_LOCAL_SEARCH_HH

#include "common.hh"
#include "incremental_solution.hh"

#include <random>

namespace minipart {

class LocalSearch {
 public:
  virtual void run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const =0;

  virtual ~LocalSearch() {}
};

template<typename ObjectiveFunction>
class GenericLocalSearch : public LocalSearch {
 public:
  void run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const final override;
 
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

template<typename ObjectiveFunction>
inline void GenericLocalSearch<ObjectiveFunction>::run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const {
  Index nMoves = params.movesPerElement * hypergraph.nNodes() * (hypergraph.nParts()-1);
  Runner runner(hypergraph, solution, params, rgen);
  runner.simpleSwaps(nMoves);
  runner.absorptionMoves(nMoves);
}

template<typename ObjectiveFunction>
inline GenericLocalSearch<ObjectiveFunction>::Runner::Runner(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen)
: inc_(hypergraph, solution)
, rgen_(rgen) {
}

template<typename ObjectiveFunction>
inline void GenericLocalSearch<ObjectiveFunction>::Runner::simpleMoves(Index nMoves) {
  std::uniform_int_distribution<int> partDist(0, inc_.nParts()-1);
  std::uniform_int_distribution<int> nodeDist(0, inc_.nNodes()-1);
  for (int iter = 0; iter < nMoves; ++iter) {
    Index node = nodeDist(rgen_);
    Index src = inc_.solution(node);
    Index dst = partDist(rgen_);

    ObjectiveFunction before(inc_);
    inc_.move(node, dst);
    ObjectiveFunction after(inc_);
    if (before < after) {
      inc_.move(node, src);
    }
  }
}

template<typename ObjectiveFunction>
inline void GenericLocalSearch<ObjectiveFunction>::Runner::simpleSwaps(Index nMoves) {
  std::uniform_int_distribution<int> nodeDist(0, inc_.nNodes()-1);
  for (int iter = 0; iter < nMoves; ++iter) {
    Index n1 = nodeDist(rgen_);
    Index n2 = nodeDist(rgen_);
    Index p1 = inc_.solution(n1);
    Index p2 = inc_.solution(n1);
    if (p1 == p2) continue;

    ObjectiveFunction before(inc_);
    inc_.move(n1, p2);
    inc_.move(n2, p1);
    ObjectiveFunction after(inc_);
    if (before < after) {
      inc_.move(n1, p1);
      inc_.move(n2, p2);
    }
  }
}

template<typename ObjectiveFunction>
inline void GenericLocalSearch<ObjectiveFunction>::Runner::absorptionMoves(Index nMoves) {
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

      ObjectiveFunction before(inc_);
      inc_.move(node, dst);
      ObjectiveFunction after(inc_);
      if (before < after) {
        inc_.move(node, src);
      }
      else {
        for (Index hEdge : inc_.hypergraph().nodeHedges(node)) {
          for (Index neighbour : inc_.hypergraph().hedgeNodes(hEdge)) {
            candidates_.push_back(neighbour);
          }
        }
      }
      ++iter;
    }
  }
}

} // End namespace minipart

#endif

