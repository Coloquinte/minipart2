// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "hypergraph.hh"
#include "partitioning_params.hh"
#include "blackbox_optimizer.hh"
#include "local_search.hh"
#include "objective_function.hh"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <boost/program_options.hpp>

using namespace std;
using namespace minipart;
namespace po = boost::program_options;

po::options_description getBaseOptions() {
  po::options_description desc("Minipart options");

  desc.add_options()("input,i", po::value<string>(),
                     "Input file name (.hgr)");

  desc.add_options()("output,o", po::value<string>(),
                     "Solution file");

  desc.add_options()("partitions,k", po::value<Index>()->default_value(2),
                     "Number of partitions");

  desc.add_options()("imbalance,e", po::value<double>()->default_value(5.0),
                     "Imbalance factor (%)");

  desc.add_options()("verbosity,v", po::value<Index>()->default_value(1),
                     "Verbosity level");

  desc.add_options()("seed,s", po::value<size_t>()->default_value(0),
                     "Random seed");

  desc.add_options()("help,h", "Print this help");

  return desc;
}

po::options_description getBlackboxOptions() {
  po::options_description desc("Algorithm options");

  desc.add_options()("pool-size", po::value<Index>()->default_value(32),
                     "Number of solutions");

  desc.add_options()("v-cycles", po::value<Index>()->default_value(5),
                     "Number of V-cycles");

  desc.add_options()("min-c-factor", po::value<double>()->default_value(1.2),
                     "Minimum reduction in coarsening");

  desc.add_options()("max-c-factor", po::value<double>()->default_value(3.0),
                     "Maximum reduction in coarsening");

  desc.add_options()("move-ratio", po::value<double>()->default_value(5.0),
                     "Number of moves per vertex");

  return desc;
}

po::variables_map parseArguments(int argc, char **argv) {
  cout << fixed << setprecision(2);
  cerr << fixed << setprecision(2);

  po::options_description baseOptions = getBaseOptions();
  po::options_description blackboxOptions = getBlackboxOptions();

  po::options_description allOptions;
  allOptions.add(baseOptions).add(blackboxOptions);
  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, allOptions), vm);
    po::notify(vm);
  } catch (po::error &e) {
    cerr << "Error parsing command line arguments: ";
    cerr << e.what() << endl << endl;
    cout << allOptions << endl;
    exit(1);
  }

  if (vm.count("help")) {
    cout << allOptions << endl;
    exit(0);
  }
  if (!vm.count("input")) {
    cout << "Missing input file" << endl << endl;
    cout << allOptions << endl;
    exit(1);
  }

  return vm;
}

int main(int argc, char **argv) {
  po::variables_map vm = parseArguments(argc, argv);

  ifstream f(vm["input"].as<string>());
  Hypergraph hg = Hypergraph::readHgr(f);
  hg.checkConsistency();
  hg.mergeParallelHedges();
  hg.setupPartitions(
    vm["partitions"].as<Index>(),
    vm["imbalance"].as<double>() / 100.0
  );

  PartitioningParams params {
    .verbosity = vm["verbosity"].as<Index>(),
    .seed = vm["seed"].as<size_t>(),
    .nSolutions = vm["pool-size"].as<Index>(),
    .nCycles = vm["v-cycles"].as<Index>(),
    .minCoarseningFactor = vm["min-c-factor"].as<double>(),
    .maxCoarseningFactor = vm["max-c-factor"].as<double>(),
    .movesPerElement = vm["move-ratio"].as<double>(),
  };

  GenericLocalSearch<ObjectiveConnectivity> localSearch;
  Solution sol = BlackboxOptimizer::run(hg, params, localSearch);
  if (vm.count("output")) {
    ofstream os(vm["output"].as<string>());
    sol.write(os);
  }



  return 0;
}

