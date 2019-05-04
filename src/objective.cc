// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "incremental_objective.hh"
#include "objective.hh"

#include <cassert>
#include <algorithm>

using namespace std;

namespace minipart {

unique_ptr<IncrementalObjective> CutObjective::incremental(const Hypergraph &h, Solution &s) {
  return make_unique<IncrementalCut>(h, s);
}

unique_ptr<IncrementalObjective> SoedObjective::incremental(const Hypergraph &h, Solution &s) {
  return make_unique<IncrementalSoed>(h, s);
}

unique_ptr<IncrementalObjective> MaxDegreeObjective::incremental(const Hypergraph &h, Solution &s) {
  return make_unique<IncrementalMaxDegree>(h, s);
}

} // End namespace minipart
