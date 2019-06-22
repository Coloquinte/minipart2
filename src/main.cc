// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "hypergraph.hh"
#include "partitioning_params.hh"
#include "blackbox_optimizer.hh"
#include "objective.hh"
#include "config.hh"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <boost/program_options.hpp>

using namespace std;
using namespace minipart;
namespace po = boost::program_options;

po::options_description getBaseOptions() {
  po::options_description desc("Options");

  desc.add_options()("hypergraph,i", po::value<string>(),
                     "Input file name (.hgr)");

  desc.add_options()("solution,o", po::value<string>(),
                     "Solution file");

  desc.add_options()("initial,f", po::value<string>(),
                     "Initial solution file");

  desc.add_options()("partitions,k", po::value<Index>()->default_value(2),
                     "Number of partitions");

  desc.add_options()("imbalance,e", po::value<double>()->default_value(5.0),
                     "Imbalance factor (%)");

  desc.add_options()("objective,g", po::value<ObjectiveType>()->default_value(ObjectiveType::Soed),
                     "Objective function: cut, soed or max-degree");

  desc.add_options()("verbosity,v", po::value<Index>()->default_value(1),
                     "Verbosity level");

  desc.add_options()("seed,s", po::value<size_t>()->default_value(0),
                     "Random seed");

  desc.add_options()("help,h", "Print this help");

  desc.add_options()("version", "Show the program version");

  return desc;
}

po::options_description getBlackboxOptions() {
  po::options_description desc("Algorithm parameters");

  desc.add_options()("pool-size", po::value<Index>()->default_value(32),
                     "Number of solutions");

  desc.add_options()("v-cycles", po::value<Index>()->default_value(5),
                     "Number of V-cycles");

  desc.add_options()("min-c-factor", po::value<double>()->default_value(1.2),
                     "Minimum coarsening factor");

  desc.add_options()("max-c-factor", po::value<double>()->default_value(3.0),
                     "Maximum coarsening factors");

  desc.add_options()("min-c-nodes", po::value<Index>()->default_value(50),
                     "Minimum nodes per partition for coarsening");

  desc.add_options()("move-ratio", po::value<double>()->default_value(5.0),
                     "Number of moves per vertex");

  return desc;
}

po::options_description getHiddenOptions() {
  po::options_description desc("Hidden options");
  desc.add_options()("no-solve", "Skip the solving phase");
  return desc;
}

po::variables_map parseArguments(int argc, char **argv) {
  cout << fixed << setprecision(1);
  cerr << fixed << setprecision(1);

  po::options_description baseOptions = getBaseOptions();
  po::options_description blackboxOptions = getBlackboxOptions();
  po::options_description hiddenOptions = getHiddenOptions();

  po::options_description visibleOptions;
  visibleOptions.add(baseOptions).add(blackboxOptions);
  po::options_description allOptions;
  allOptions.add(baseOptions).add(blackboxOptions).add(hiddenOptions);

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, allOptions), vm);
    po::notify(vm);
  } catch (po::error &e) {
    cerr << "Error parsing command line arguments: ";
    cerr << e.what() << endl << endl;
    cout << visibleOptions << endl;
    exit(1);
  }

  if (vm.count("help")) {
    cout << "Minipart " << MINIPART_VERSION_NUMBER << endl;
    cout << visibleOptions << endl;
    exit(0);
  }
  if (vm.count("version")) {
    cout << "Minipart-" << MINIPART_VERSION_NUMBER << "@" << MINIPART_BUILD_DATE << endl;
    cout << "Version " << MINIPART_VERSION_NUMBER << endl;
    cout << "Built " << MINIPART_BUILD_DATE << endl;
    exit(0);
  }
  if (!vm.count("hypergraph")) {
    cout << "Missing input file" << endl << endl;
    cout << visibleOptions << endl;
    exit(1);
  }

  return vm;
}

Hypergraph readHypergraph(const po::variables_map &vm) {
  ifstream f(vm["hypergraph"].as<string>());
  Hypergraph hg = Hypergraph::readHgr(f);
  hg.checkConsistency();
  hg.mergeParallelHedges();
  hg.setupPartitions(
    vm["partitions"].as<Index>(),
    vm["imbalance"].as<double>() / 100.0
  );
  return hg;
}

PartitioningParams readParams(const po::variables_map &vm, const Hypergraph &hg) {
  return PartitioningParams {
    .verbosity = vm["verbosity"].as<Index>(),
    .seed = vm["seed"].as<size_t>(),
    .objective = vm["objective"].as<ObjectiveType>(),
    .nSolutions = vm["pool-size"].as<Index>(),
    .nCycles = vm["v-cycles"].as<Index>(),
    .minCoarseningFactor = vm["min-c-factor"].as<double>(),
    .maxCoarseningFactor = vm["max-c-factor"].as<double>(),
    .minCoarseningNodes = vm["min-c-nodes"].as<Index>(),
    .movesPerElement = vm["move-ratio"].as<double>(),
    .nNodes = hg.nNodes(),
    .nHedges = hg.nHedges(),
    .nPins = hg.nPins(),
    .nParts = hg.nParts()
  };
}

vector<Solution> readInitialSolutions(const po::variables_map &vm, const Hypergraph &hg) {
  vector<Solution> solutions;
  if (vm.count("initial")) {
    ifstream f(vm["initial"].as<string>());
    Solution sol = Solution::read(f);
    sol.checkConsistency();
    solutions.emplace_back(sol);
  }
  return solutions;
}

void writeFinalSolution(const po::variables_map &vm, const Solution &solution) {
  if (vm.count("solution")) {
    ofstream os(vm["solution"].as<string>());
    solution.write(os);
  }
}

void report(const PartitioningParams &, const Hypergraph &hg) {
  cout << "Nodes: " << hg.nNodes() << endl;
  cout << "Edges: " << hg.nHedges() << endl;
  cout << "Pins: " << hg.nPins() << endl;
  cout << "Parts: " << hg.nParts() << endl;
  cout << endl;
}

void reportMainMetrics(const PartitioningParams &params, const Hypergraph &hg, const Solution &sol) {
  cout << "Cut: " << hg.metricsCut(sol) << endl;
  if (hg.nParts() > 2) {
    cout << "Connectivity: " << hg.metricsConnectivity(sol) << endl;
    cout << "Maximum degree: " << hg.metricsMaxDegree(sol) << endl;
  }
  if (params.isDaisyChainObj()) {
    if (hg.nParts() > 2) {
      cout << "Daisy-chain distance: " << hg.metricsDaisyChainDistance(sol) << endl;
      cout << "Daisy-chain maximum degree: " << hg.metricsDaisyChainMaxDegree(sol) << endl;
    }
  }
  if (params.isRatioObj()) {
    cout << "Ratio cut: " << hg.metricsRatioCut(sol) << endl;
    if (hg.nParts() > 2) {
      cout << "Ratio connectivity: " << hg.metricsRatioConnectivity(sol) << endl;
      cout << "Ratio maximum degree: " << hg.metricsRatioMaxDegree(sol) << endl;
    }
    cout << "Ratio penalty: " << 100.0 * (hg.metricsRatioPenalty(sol) - 1.0) << "%" << endl;
  }
  cout << endl;
}

void reportPartitionUsage(const PartitioningParams &params, const Hypergraph &hg, const Solution &sol) {
  std::vector<Index> usage  = hg.metricsPartitionUsage(sol);
  if (params.isRatioObj()) {
    Index totNodeWeight = hg.totalNodeWeight();
    cout << "Partition usage:" << endl;
    for (Index p = 0; p < hg.nParts(); ++p) {
      cout << "\tPart#" << p << "  \t";
      cout << usage[p] << "\t";
      cout << "(" << 100.0 * usage[p] / totNodeWeight << "%)";
      cout << endl;
    }
    cout << endl;
  }
  else {
    cout << "Partition usage:" << endl;
    for (Index p = 0; p < hg.nParts(); ++p) {
      cout << "\tPart#" << p << "  \t";
      cout << usage[p] << "\t/ " << hg.partWeight(p) << "\t";
      cout << "(" << 100.0 * usage[p] / hg.partWeight(p) << "%)\t";
      if (usage[p] > hg.partWeight(p)) cout << "(overflow)";
      cout << endl;
    }
    cout << endl;
  }
}

void reportPartitionDegree(const PartitioningParams &params, const Hypergraph &hg, const Solution &sol) {
  if (hg.nParts() <= 2) return;

  std::vector<Index> degree = hg.metricsPartitionDegree(sol);
  cout << "Partition degrees:" << endl;
  for (Index p = 0; p < hg.nParts(); ++p) {
    cout << "\tPart#" << p << "  \t";
    cout << degree[p] << endl;
  }
  if (params.isDaisyChainObj()) {
    cout << endl;
    cout << "Daisy-chain partition degrees: " << endl;
    degree = hg.metricsPartitionDaisyChainDegree(sol);
    for (Index p = 0; p < hg.nParts(); ++p) {
      cout << "\tPart#" << p << "  \t";
      cout << degree[p] << endl;
    }
  }
}

void report(const PartitioningParams &params, const Hypergraph &hg, const Solution &sol) {
  reportMainMetrics(params, hg, sol);
  reportPartitionUsage(params, hg, sol);
  reportPartitionDegree(params, hg, sol);
}

unique_ptr<Objective> readObjective(const po::variables_map &vm) {
  switch (vm["objective"].as<ObjectiveType>()) {
    case ObjectiveType::Cut:
      return make_unique<CutObjective>();
    case ObjectiveType::Soed:
      return make_unique<SoedObjective>();
    case ObjectiveType::MaxDegree:
      return make_unique<MaxDegreeObjective>();
    case ObjectiveType::DaisyChainDistance:
      return make_unique<DaisyChainDistanceObjective>();
    case ObjectiveType::DaisyChainMaxDegree:
      return make_unique<DaisyChainMaxDegreeObjective>();
    case ObjectiveType::RatioCut:
      return make_unique<RatioCutObjective>();
    case ObjectiveType::RatioSoed:
      return make_unique<RatioSoedObjective>();
    case ObjectiveType::RatioMaxDegree:
      return make_unique<RatioMaxDegreeObjective>();
    default:
      cout << "Objective type is not supported." << endl;
      exit(1);
  }
}

void initialReport(const Hypergraph &hg, const PartitioningParams &params, const vector<Solution> &initialSolutions) {
  if (params.verbosity >= 1) {
    report(params, hg);
    if (initialSolutions.size() > 0) {
      cout << "Initial solution:" << endl;
    }
    for (const Solution &sol : initialSolutions) {
      report(params, hg, sol);
    }
  }
}

void finalReport(const Hypergraph &hg, const PartitioningParams &params, const vector<Solution> &finalSolutions) {
  if (params.verbosity >= 1) {
    for (const Solution &sol : finalSolutions) {
      report(params, hg, sol);
    }
  }
}

int main(int argc, char **argv) {
  po::variables_map vm = parseArguments(argc, argv);

  Hypergraph hg = readHypergraph(vm);
  PartitioningParams params = readParams(vm, hg);
  unique_ptr<Objective> objectivePtr = readObjective(vm);
  vector<Solution> initialSolutions = readInitialSolutions(vm, hg);

  initialReport(hg, params, initialSolutions);
  if (vm.count("no-solve")) return 0;

  Solution solution = BlackboxOptimizer::run(hg, params, *objectivePtr, initialSolutions);
  finalReport(hg, params, {solution});
  writeFinalSolution(vm, solution);
  return 0;
}

