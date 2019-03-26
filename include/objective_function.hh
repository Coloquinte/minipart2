
#ifndef MINIPART_OBJECTIVE_FUNCTION_HH
#define MINIPART_OBJECTIVE_FUNCTION_HH

/**
 * Basic objective functions for use during local search
 */

#include "hypergraph.hh"
#include "solution.hh"
#include "incremental_solution.hh"

namespace minipart {

class ObjectiveConnectivity {
 public:
  ObjectiveConnectivity(const Hypergraph &h, const Solution &s) {
    overflow_ = h.metricsSumOverflow(s);
    connectivity_ = h.metricsConnectivity(s);
  }

  ObjectiveConnectivity(const IncrementalSolution &inc) {
    overflow_ = inc.metricsSumOverflow();
    connectivity_ = inc.metricsConnectivity();
  }

  bool operator<(const ObjectiveConnectivity &o) const {
    if (overflow_ != o.overflow_)
      return overflow_ < o.overflow_;
    return connectivity_ < o.connectivity_;
  }

  void report() const;

 private:
  Index overflow_;
  Index connectivity_;
};

class ObjectiveCut {
 public:
  ObjectiveCut(const Hypergraph &h, const Solution &s) {
    overflow_ = h.metricsSumOverflow(s);
    cut_ = h.metricsCut(s);
    connectivity_ = h.metricsConnectivity(s);
  }

  ObjectiveCut(const IncrementalSolution &inc) {
    overflow_ = inc.metricsSumOverflow();
    cut_ = inc.metricsCut();
    connectivity_ = inc.metricsConnectivity();
  }

  bool operator<(const ObjectiveCut &o) const {
    if (overflow_ != o.overflow_)
      return overflow_ < o.overflow_;
    if (connectivity_ != o.connectivity_)
      return connectivity_ < o.connectivity_;
    return cut_ < o.cut_;
  }

  void report() const;

 private:
  Index overflow_;
  Index cut_;
  Index connectivity_;
};

} // End namespace minipart

#endif

