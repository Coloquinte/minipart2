
#include "objective_function.hh"

#include <iostream>

using namespace std;

namespace minipart {

void ObjectiveConnectivity::report() const {
  if (overflow_ != 0)
    cout << "Overflow: " << overflow_ << ", connectivity: " << connectivity_;
  else
    cout << "Connectivity: " << connectivity_;
}

void ObjectiveCut::report() const {
  if (overflow_ != 0)
    cout << "Overflow: " << overflow_ << ", cut: " << cut_;
  else
    cout << "Cut: " << cut_ << ", connectivity: " << connectivity_;
}

} // End namespace minipart

