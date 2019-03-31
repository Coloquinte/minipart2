
#include "objective_function.hh"

#include <iostream>

using namespace std;

namespace minipart {

void SoedObjectiveType::report(ostream &os) const {
  if (overflow_ != 0)
    os << "Overflow: " << overflow_ << ", connectivity: " << connectivity_;
  else
    os << "Connectivity: " << connectivity_;
}

void CutObjectiveType::report(ostream &os) const {
  if (overflow_ != 0)
    os << "Overflow: " << overflow_ << ", cut: " << cut_;
  else
    os << "Cut: " << cut_ << ", connectivity: " << connectivity_;
}

void MaxDegreeObjectiveType::report(ostream &os) const {
  if (overflow_ != 0)
    os << "Overflow: " << overflow_ << ", max degree: " << maxDegree_;
  else
    os << "Max degree: " << maxDegree_ << ", connectivity: " << connectivity_;
}

} // End namespace minipart

