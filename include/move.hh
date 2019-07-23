
#ifndef MINIPART_MOVE_HH
#define MINIPART_MOVE_HH

#include "common.hh"

#include <random>

namespace minipart {

class Move {
 public:
  Move(Index budget) : budget_(budget) {}
  virtual void run(IncrementalObjective &inc, std::mt19937 &rgen) =0;
  virtual ~Move() {}

  std::int64_t budget_;
};

class VertexMoveRandomBlock : public Move {
 public:
  VertexMoveRandomBlock(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class VertexMoveBestBlock : public Move {
 public:
  VertexMoveBestBlock(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class VertexPassRandomBlock : public Move {
 public:
  VertexPassRandomBlock(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class VertexPassBestBlock : public Move {
 public:
  VertexPassBestBlock(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class EdgeMoveRandomBlock : public Move {
 public:
  EdgeMoveRandomBlock(Index budget) : Move(budget) {
    edgeDegreeCutoff_ = 10;
  }
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;

 private:
  std::vector<std::pair<Index, Index> > initialStatus_;
  std::size_t edgeDegreeCutoff_;
};

class VertexSwap : public Move {
 public:
  VertexSwap(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class VertexAbsorptionPass : public Move {
 public:
  VertexAbsorptionPass(Index budget) : Move(budget) {
    nodeDegreeCutoff_ = 10;
    edgeDegreeCutoff_ = 10;
  }
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;

 private:
  std::vector<Index> candidates_;
  std::size_t nodeDegreeCutoff_;
  std::size_t edgeDegreeCutoff_;
};

} // End namespace minipart

#endif

