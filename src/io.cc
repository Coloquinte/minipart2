
#include "hypergraph.hh"

#include <sstream>
#include <iostream>

using namespace std;

namespace minipart {

namespace {

stringstream getUncommentedLine(istream &s) {
  // Get rid of comment lines and empty lines
  string tmp;
  do {
    if (!s.good()) throw runtime_error("Not enough lines");
    getline(s, tmp);
  } while(tmp.empty() || tmp[0] == '%');

  return stringstream(tmp);
}

} // End anonymous namespace


Hypergraph Hypergraph::readHmetis(istream &s) {
  Index nNodes, nHedges, params;

  stringstream ss = getUncommentedLine(s);
  ss >> nHedges >> nNodes;
  if (ss.fail()) throw runtime_error("Invalid first line");
  ss >> params;
  if (ss.fail()) params = 0;

  if (params != 0 && params != 1 && params != 10 && params != 11) throw runtime_error("Invalid parameter value");

  bool hasHedgeWeights = (params == 11) || (params == 1);
  bool hasNodeWeights = (params == 11) || (params == 10);

  Hypergraph ret;

  // Read edges
  ret.hedgeWeights_.reserve(nHedges);
  ret.hedgeToNodes_.reserve(nHedges);
  vector<Index> nodes;
  for (Index i = 0; i < nHedges; ++i) {
    ss = getUncommentedLine(s);

    Index w = 1;
    if (hasHedgeWeights) ss >> w;

    while (ss) {
        Index n;
        ss >> n;
        if (ss.fail()) continue;
        if (n > nNodes) throw runtime_error("Parsed pin index is outside the specified number of nodes");
        if (n == 0) throw runtime_error("Parsed pin index cannot be 0");
        nodes.push_back(n-1);
    }
    if (nodes.empty()) throw runtime_error("No node on the line");
    ret.hedgeWeights_.push_back(w);
    ret.hedgeToNodes_.push_back(nodes);
    nodes.clear();
  }

  // Read node weights
  ret.nodeWeights_.reserve(nNodes);
  if (hasNodeWeights) {
    vector<Index> demands;
    for (Index i = 0; i < nNodes; ++i) {
      ss = getUncommentedLine(s);
      while (ss) {
        Index w;
        ss >> w;
        if (ss.fail()) continue;
        demands.push_back(w);
      }
      if (demands.size() != 1) throw std::runtime_error("All nodes should have exactly one weight");
      ret.nodeWeights_.push_back(demands.front());
      demands.clear();
    }
  }
  else {
    ret.nodeWeights_.assign(nNodes, 1);
  }

  // Finalize nodes 
  ret.constructNodes();

  return ret;
}

void Hypergraph::writeHmetis(ostream &s) const {
  s << "% HGR (hMetis) file generated by Minipart\n";
  s << "% " << nNodes() << " nodes, " << nHedges() << " hyperedges\n";

  s << nHedges() << " " << nNodes() << " 11\n";
  for (Index hedge = 0; hedge < nHedges(); ++hedge) {
    s << hedgeWeight(hedge);
    for (Index node : hedgeNodes(hedge)) {
      s << " " << node + 1;
    }
    s << "\n";
  }
  for (Index node = 0; node < nNodes(); ++node) {
    s << nodeWeight(node) << "\n";
  }
}

} // End namespace minipart

