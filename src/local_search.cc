
#include "generic_local_search.hh"
#include "objective_function.hh"

using namespace std;

namespace minipart {
unique_ptr<LocalSearch> LocalSearch::cut() {
  return make_unique<GenericLocalSearch<CutObjectiveType> >();
}

unique_ptr<LocalSearch> LocalSearch::soed() {
  return make_unique<GenericLocalSearch<SoedObjectiveType> >();
}

} // End namespace minipart

