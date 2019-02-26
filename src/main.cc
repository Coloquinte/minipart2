
#include "hypergraph.hh"
#include "blackbox_optimizer.hh"

#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;
using namespace minipart;

int main(int argc, char **argv) {
  if (argc < 3) {
    cout << "Usage: minipart hypergraph.hgr nbPartitions" << endl;
    exit(1);
  }

  ifstream f(argv[1]);
  Hypergraph hg = Hypergraph::readHmetis(f);
  hg.checkConsistency();
  hg.mergeParallelHedges();

  BlackboxOptimizer::Params params {
    .nPartitions = atoi(argv[2]),
    .imbalanceFactor = 0.05,
    .nCycles = 1,
    .coarseningFactor = 2.0,
    .movesPerElement = 4.0,
    .seed = 1
  };
  BlackboxOptimizer::run(hg, params);

  ofstream of("test.hgr");
  hg.writeHmetis(of);
  
  return 0;
}

