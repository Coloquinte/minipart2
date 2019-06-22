// Copyright (C) 2019 Gabriel Gouvine - All Rights Reserved

#include "partitioning_params.hh"

#include <iostream>

using namespace std;

namespace minipart {

std::istream &operator>>(std::istream &is, ObjectiveType &obj) {
  std::string token;
  is >> token;
  if (token == "cut")
    obj = ObjectiveType::Cut;
  else if (token == "soed" || token == "connectivity")
    obj = ObjectiveType::Soed;
  else if (token == "max-degree")
    obj = ObjectiveType::MaxDegree;
  else if (token == "daisy-chain-distance")
    obj = ObjectiveType::DaisyChainDistance;
  else if (token == "daisy-chain-max-degree")
    obj = ObjectiveType::DaisyChainMaxDegree;
  else if (token == "ratio-cut")
    obj = ObjectiveType::RatioCut;
  else if (token == "ratio-soed" || token == "ratio-connectivity")
    obj = ObjectiveType::RatioSoed;
  else if (token == "ratio-max-degree")
    obj = ObjectiveType::RatioMaxDegree;
  else
    is.setstate(std::ios_base::failbit);
  return is;
}

std::ostream &operator<<(std::ostream &os, const ObjectiveType &obj) {
  switch (obj) {
    case ObjectiveType::Cut:
      os << "cut";
      break;
    case ObjectiveType::Soed:
      os << "soed";
      break;
    case ObjectiveType::MaxDegree:
      os << "max-degree";
      break;
    case ObjectiveType::DaisyChainDistance:
      os << "daisy-chain-distance";
      break;
    case ObjectiveType::DaisyChainMaxDegree:
      os << "daisy-chain-max-degree";
      break;
    case ObjectiveType::RatioCut:
      os << "ratio-cut";
      break;
    case ObjectiveType::RatioSoed:
      os << "ratio-soed";
      break;
    case ObjectiveType::RatioMaxDegree:
      os << "ratio-max-degree";
      break;
  }
  return os;
}

bool PartitioningParams::isRatioObj() const {
  return objective == ObjectiveType::RatioCut || objective == ObjectiveType::RatioSoed || objective == ObjectiveType::RatioMaxDegree;
}

bool PartitioningParams::isDaisyChainObj() const {
  return objective == ObjectiveType::DaisyChainMaxDegree || objective == ObjectiveType::DaisyChainDistance;
}

} // End namespace minipart

