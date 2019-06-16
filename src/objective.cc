// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "incremental_objective.hh"
#include "objective.hh"

#include <cassert>
#include <algorithm>

using namespace std;

namespace minipart {

unique_ptr<IncrementalObjective> CutObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalCut>(h, s);
}

unique_ptr<IncrementalObjective> SoedObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalSoed>(h, s);
}

unique_ptr<IncrementalObjective> MaxDegreeObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalMaxDegree>(h, s);
}

unique_ptr<IncrementalObjective> DaisyChainDistanceObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalDaisyChainDistance>(h, s);
}

unique_ptr<IncrementalObjective> DaisyChainMaxDegreeObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalDaisyChainMaxDegree>(h, s);
}

unique_ptr<IncrementalObjective> RatioCutObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalRatioCut>(h, s);
}

unique_ptr<IncrementalObjective> RatioSoedObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalRatioSoed>(h, s);
}

unique_ptr<IncrementalObjective> RatioMaxDegreeObjective::incremental(const Hypergraph &h, Solution &s) const {
  return make_unique<IncrementalRatioMaxDegree>(h, s);
}

vector<int64_t> CutObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsSumOverflow(s), h.metricsCut(s), h.metricsConnectivity(s) };
}

vector<int64_t> SoedObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsSumOverflow(s), h.metricsConnectivity(s) };
}

vector<int64_t> MaxDegreeObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsSumOverflow(s), h.metricsMaxDegree(s), h.metricsConnectivity(s) };
}

vector<int64_t> DaisyChainDistanceObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsSumOverflow(s), h.metricsDaisyChainDistance(s), h.metricsConnectivity(s) };
}

vector<int64_t> DaisyChainMaxDegreeObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsSumOverflow(s), h.metricsDaisyChainMaxDegree(s), h.metricsDaisyChainDistance(s) };
}

vector<int64_t> RatioCutObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsEmptyPartitions(s), (int64_t) 100.0 * h.metricsRatioCut(s), h.metricsCut(s), h.metricsConnectivity(s) };
}

vector<int64_t> RatioSoedObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsEmptyPartitions(s), (int64_t) 100.0 * h.metricsRatioSoed(s), h.metricsConnectivity(s) };
}

vector<int64_t> RatioMaxDegreeObjective::eval(const Hypergraph &h, Solution &s) const {
  return { h.metricsEmptyPartitions(s), (int64_t) 100.0 * h.metricsRatioMaxDegree(s), h.metricsConnectivity(s) };
}

} // End namespace minipart
