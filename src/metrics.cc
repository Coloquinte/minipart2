// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "hypergraph.hh"

#include <cassert>
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <cmath>

using namespace std;

namespace minipart {

Index Hypergraph::metricsCut(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nHedgeWeights() == 1);
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    if (cut(solution, hedge))
      ret += hedgeWeight(hedge);
  }
  return ret;
}

Index Hypergraph::metricsSoed(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nHedgeWeights() == 1);
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge) * degree(solution, hedge);
  }
  return ret;
}

Index Hypergraph::metricsConnectivity(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nHedgeWeights() == 1);
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    ret += hedgeWeight(hedge) * (degree(solution, hedge) - 1);
  }
  return ret;
}

Index Hypergraph::metricsDaisyChainDistance(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nHedgeWeights() == 1);
  Index ret = 0;
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    Index minPart = nParts() - 1;
    Index maxPart = 0;
    for (Index node : hedgeNodes(hedge)) {
      minPart = min(minPart, solution[node]);
      maxPart = max(maxPart, solution[node]);
    }
    if (minPart >= maxPart) continue;
    ret += hedgeWeight(hedge) * (maxPart - minPart);
  }
  return ret;
}

Index Hypergraph::metricsSumOverflow(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nNodeWeights() == 1);
  vector<Index> usage = metricsPartitionUsage(solution);
  Index ret = 0;
  for (int i = 0; i < nParts(); ++i) {
    Index ovf = usage[i] - partData_[i];
    if (ovf > 0)
      ret += ovf;
  }
  return ret;
}

Index Hypergraph::metricsMaxDegree(const Solution &solution) const {
  vector<Index> degree = metricsPartitionDegree(solution);
  return *max_element(degree.begin(), degree.end());
}

Index Hypergraph::metricsDaisyChainMaxDegree(const Solution &solution) const {
  vector<Index> degree = metricsPartitionDaisyChainDegree(solution);
  return *max_element(degree.begin(), degree.end());
}

double Hypergraph::metricsRatioPenalty(const Solution &solution) const {
  vector<Index> partitionUsage = metricsPartitionUsage(solution);
  Index sumUsage = 0;
  for (Index d : partitionUsage)
    sumUsage += d;
  double normalizedUsage = ((double) sumUsage) / partitionUsage.size();
  double productUsage = 1.0;
  for (Index d : partitionUsage) {
    productUsage *= (d / normalizedUsage);
  }
  // Geomean squared
  return 1.0 / pow(productUsage, 2.0 / partitionUsage.size());
}

Index Hypergraph::metricsEmptyPartitions(const Solution &solution) const {
  vector<Index> partitionUsage = metricsPartitionUsage(solution);
  Index count = 0;
  for (Index d : partitionUsage) {
    if (d == 0) count++;
  }
  return count;
}

double Hypergraph::metricsRatioCut(const Solution &solution) const {
  return metricsCut(solution) * metricsRatioPenalty(solution);
}

double Hypergraph::metricsRatioSoed(const Solution &solution) const {
  return metricsSoed(solution) * metricsRatioPenalty(solution);
}

double Hypergraph::metricsRatioConnectivity(const Solution &solution) const {
  return metricsConnectivity(solution) * metricsRatioPenalty(solution);
}

double Hypergraph::metricsRatioMaxDegree(const Solution &solution) const {
  return metricsMaxDegree(solution) * metricsRatioPenalty(solution);
}

std::vector<Index> Hypergraph::metricsPartitionUsage(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  vector<Index> usage(nParts(), 0);
  for (int i = 0; i < nNodes(); ++i) {
    assert (solution[i] >= 0 && solution[i] < nParts());
    usage[solution[i]] += nodeWeight(i);
  }
  return usage;
}

std::vector<Index> Hypergraph::metricsPartitionDegree(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nHedgeWeights() == 1);
  vector<Index> degree(nParts(), 0);
  unordered_set<Index> parts;
  for (int i = 0; i < nHedges(); ++i) {
    parts.clear();
    for (Index node : hedgeNodes(i)) {
      parts.insert(solution[node]);
    }
    if (parts.size() > 1) {
      for (Index p : parts) {
        degree[p] += hedgeWeight(i);
      }
    }
  }
  return degree;
}

std::vector<Index> Hypergraph::metricsPartitionDaisyChainDegree(const Solution &solution) const {
  assert (solution.nNodes() == nNodes());
  assert (solution.nParts() == nParts());
  assert (nHedgeWeights() == 1);
  vector<Index> degree(nParts(), 0);
  unordered_set<Index> parts;
  for (int i = 0; i < nHedges(); ++i) {
    Index minPart = nParts() - 1;
    Index maxPart = 0;
    for (Index node : hedgeNodes(i)) {
      minPart = min(minPart, solution[node]);
      maxPart = max(maxPart, solution[node]);
    }
    if (minPart >= maxPart) continue;
    // Count twice for middle partitions
    for (Index p = minPart; p < maxPart; ++p) {
      degree[p] += hedgeWeight(i);
      degree[p+1] += hedgeWeight(i);
    }
  }
  return degree;
}

} // End namespace minipart

