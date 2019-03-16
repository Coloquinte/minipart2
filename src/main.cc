// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "hypergraph.hh"
#include "partitioning_params.hh"
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

  PartitioningParams params {
    .nPartitions = atoi(argv[2]),
    .imbalanceFactor = 0.05,
  };

  ifstream f(argv[1]);
  Hypergraph hg = Hypergraph::readHmetis(f);
  hg.checkConsistency();
  hg.mergeParallelHedges();
  hg.setupPartitions(params.nPartitions, params.imbalanceFactor);

  BlackboxOptimizer::Params optParams {
    .nSolutions = 32,
    .nCycles = 64,
    .minCoarseningFactor = 1.5,
    .maxCoarseningFactor = 4.0,
    .movesPerElement = 20.0,
    .seed = 1
  };
  BlackboxOptimizer::run(hg, optParams);

  ofstream of("test.hgr");
  hg.writeHmetis(of);
  
  return 0;
}

