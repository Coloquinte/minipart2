
#include "objective_function.hh"

#include <iostream>

using namespace std;

namespace minipart {

void ObjectiveConnectivity::report() const {
  if (overflow_ != 0)
    cout << "Overflow: " << overflow_ << endl;
  cout << "Connectivity: " << connectivity_ << endl;
}

void ObjectiveCut::report() const {
  if (overflow_ != 0)
    cout << "Overflow: " << overflow_ << endl;
  cout << "Cut: " << cut_ << endl;
}

} // End namespace minipart

