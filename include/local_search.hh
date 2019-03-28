
#ifndef MINIPART_LOCAL_SEARCH_HH
#define MINIPART_LOCAL_SEARCH_HH

#include "common.hh"

#include <random>
#include <iosfwd>
#include <memory>

namespace minipart {

class LocalSearch {
 public:
  virtual void run(const Hypergraph &hypergraph, Solution &solution, const PartitioningParams &params, std::mt19937 &rgen) const =0;
  virtual bool compare(const Hypergraph &hypergraph, const Solution &sol1, const Solution &sol2) const =0;
  virtual void report(const Hypergraph &hypergraph, const Solution &solution, std::ostream &os) const {}
  virtual ~LocalSearch() {}

  static std::unique_ptr<LocalSearch> cut();
  static std::unique_ptr<LocalSearch> soed();
};

} // End namespace minipart

#endif

