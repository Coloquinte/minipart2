
#ifndef MINIPART_MOVE_HH
#define MINIPART_MOVE_HH

#include "incremental_objective.hh"

#include <random>

namespace minipart {

class Move {
 public:
  Move(Index budget) : budget_(budget) {}
  virtual void run(IncrementalObjective &inc, std::mt19937 &rgen) =0;
  virtual ~Move() {}

  std::int64_t budget_;
};

class SimpleMove : public Move {
 public:
  SimpleMove(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class EdgeMove : public Move {
 public:
  EdgeMove(Index budget) : Move(budget) {
    edgeDegreeCutoff_ = 10;
  }
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;

 private:
  std::vector<std::pair<Index, Index> > initialStatus_;
  std::size_t edgeDegreeCutoff_;
};

class SimpleSwap : public Move {
 public:
  SimpleSwap(Index budget) : Move(budget) {}
  void run(IncrementalObjective &inc, std::mt19937 &rgen) override;
};

class AbsorptionMove : public Move {
 public:
  AbsorptionMove(Index budget) : Move(budget) {
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

