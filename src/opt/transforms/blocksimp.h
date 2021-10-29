#ifndef LAVA_BLOCKSIMP_H
#define LAVA_BLOCKSIMP_H

#include <algorithm>

#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

namespace lava::opt {
/*
 reference from <Engineering a compiler> 10.2.2
 simplify the structure of basic blocks
*/
class BlockSimplification : public FunctionPass {
private:
  bool        _changed;
  SSAPtr      _entry;
  unsigned    _count;
  BlockWalker _blkWalker;

public:
  bool runOnFunction(const FuncPtr &F) final;

  void OnePass(const FuncPtr &F);

  /* 1. Fold a Redundant Branch */
  void FoldRedundantBranch(const FuncPtr &F);

  /* 2. Combine blocks */
  void CombineBlocks(const FuncPtr &F);

  /* 3. Remove an Empty Block */
  void RemoveEmptyBlock(const FuncPtr &F);

  void UnreachableBlockElimination(const FuncPtr &F);

  void ReplaceSuccessor(BlockPtr &predecessor, BlockPtr &successor, BasicBlock *cur);

  void MergeBlocks(BasicBlock *pred, BasicBlock *succ);

  /* Remove value from phi-node in the successors */
  void RemovePhiNode(BasicBlock *block, std::vector<BasicBlock *> successors);

  void CleanUp(const FuncPtr &F);

  void initialize() final;
};

class BlockSimplificationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<BlockSimplification>();
    auto passinfo = std::make_shared<PassInfo>(pass, "BlockSimplification", false, 0, BLOCK_SIMPLIFICATION);
    return passinfo;
  }
};
}

#endif //LAVA_BLOCKSIMP_H
