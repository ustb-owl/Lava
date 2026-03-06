#include "blkwalker.h"

#include "lib/debug.h"

namespace lava::opt {

void BlockWalker::TraverseRPO(BasicBlock *BB) {
  // return if visited
  if (!_visited.insert(BB).second)
    return;

  auto termInst = BB->insts().back();
  if (auto jumpInst = dyn_cast<JumpInst>(termInst)) {
    // visit its successor
    TraverseRPO(jumpInst->target().get());
  } else if (auto branchInst = dyn_cast<BranchInst>(termInst)) {
    // visit true_block and false_block
    TraverseRPO(branchInst->true_block().get());
    TraverseRPO(branchInst->false_block().get());
  } else if (auto retInst = dyn_cast<ReturnInst>(termInst)) {
    // do nothing, because this is the end of the function
  } else {
    ERROR("unknown terminate instruction");
  }

  _rpo.push_front(BB);
}

void BlockWalker::TraversePO(BasicBlock *BB) {
  // return if visited
  if (!_visited.insert(BB).second)
    return;

  auto termInst = BB->insts().back();
  if (auto jumpInst = dyn_cast<JumpInst>(termInst)) {
    // visit its successor
    TraversePO(jumpInst->target().get());
  } else if (auto branchInst = dyn_cast<BranchInst>(termInst)) {
    // visit true_block and false_block
    TraversePO(branchInst->true_block().get());
    TraversePO(branchInst->false_block().get());
  } else if (auto retInst = dyn_cast<ReturnInst>(termInst)) {
    // do nothing, because this is the end of the function
  } else {
    ERROR("unknown terminate instruction");
  }

  _po.push_back(BB);
}

BasicBlock *BlockWalker::GetExitBlock(const std::list<BasicBlock *> &bb_list) {
  for (const auto &BB : bb_list) {
    for (const auto &it : BB->insts()) {
      if (it->classId() == ClassId::ReturnInstId)
        return BB;
    }
  }
  ERROR("should not reach here");
  return nullptr;
}

} // namespace lava::opt
