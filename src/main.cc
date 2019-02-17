
#include "hypergraph.hh"

#include <iostream>
#include <fstream>

using namespace std;
using namespace minipart;

int main(int argc, char **argv) {
  if (argc < 3) {
    cout << "Usage: minipart hypergraph.hgr nbPartitions" << endl;
    exit(1);
  }

  ifstream f(argv[1]);
  Hypergraph hg = Hypergraph::readHmetis(f);

  ofstream of("test.hgr");
  hg.writeHmetis(of);
  
  return 0;
}

