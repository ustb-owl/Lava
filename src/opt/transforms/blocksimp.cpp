
#include "blocksimp.h"

int BlockMerge;

namespace lava::opt {


bool BlockSimplification::runOnFunction(const FuncPtr &F) {
  static_cast<void>(F);
  _changed = false;
  return false;
}

void BlockSimplification::OnePass(const FuncPtr &F) {
  FoldRedundantBranch(F);
  if (_changed) {
    RebuildPredecessors(F);
    return;
  }
  CombineBlocks(F);
  if (_changed) {
    RebuildPredecessors(F);
    return;
  }
  RemoveEmptyBlock(F);
  if (_changed) {
    RebuildPredecessors(F);
    return;
  }
  UnreachableBlockElimination(F);
  RebuildPredecessors(F);
}

/*
 * If Clean finds a block that ends in a branch, and both sides of the branch target the same block,
 * it replaces the branch with a jump to the target block.
 * This situation arises as the result of other simplifications.
 * For example, Bi might have had two successors, each with a jump to B j .
 * If another transformation had already emptied those blocks, then empty-block removal,
 * discussed next, might produce the initial graph shown in the margin.
 */
void BlockSimplification::FoldRedundantBranch(const FuncPtr &F) {
  std::vector<BlockPtr> blocks;
  blocks.reserve(F->size());
  for (const auto &bb_use : *F) {
    auto block = dyn_cast<BasicBlock>(bb_use.value());
    if (block) blocks.push_back(block);
  }

  for (const auto &block : blocks) {
    auto *BB = block.get();
    if (auto branchInst = dyn_cast<BranchInst>(BB->terminator())) {

      // get condition
      auto cond = branchInst->cond();

      // if true block == false block, then replace the branch with jump
      if (branchInst->true_block() == branchInst->false_block()) {
        // create jump instruction
        auto jump = std::make_shared<JumpInst>(branchInst->true_block());
        jump->set_logger(branchInst->logger());

        branchInst->EraseFromParent();
        BB->AppendInst(jump);

        // set changed flag
        _changed = true;
        return;
      }
      else if (cond->IsConst()) {
        BlockPtr target = nullptr, discord = nullptr;
        // check if condition is true
        if (dyn_cast<ConstantInt>(cond)->IsZero()) {
          target = dyn_cast<BasicBlock>(branchInst->false_block());
          discord = dyn_cast<BasicBlock>(branchInst->true_block());
        } else {
          target = dyn_cast<BasicBlock>(branchInst->true_block());
          discord = dyn_cast<BasicBlock>(branchInst->false_block());
        }

        auto jump_inst = std::make_shared<JumpInst>(target);
        jump_inst->set_logger(branchInst->logger());
        branchInst->EraseFromParent();
        BB->AppendInst(jump_inst);

        // remove phi-node
        RemovePhiNode(BB, std::vector<BasicBlock *>{discord.get()});

        // remove current block from discord block's predecessor list
        // this block will be sweep in the following pass
        discord->RemovePredecessor(BB);

        // set changed flag
        _changed = true;
        return;

      }
    }
  }
}

/*
 * If Clean finds a block Bi that ends in a jump to B j and B j has only one predecessor,
 * it can combine the two blocks, as shown in the margin.
 * This situation can arise in several ways.
 * Another transformation might eliminate other edges that entered B j , or Bi and
 * B j might be the result of folding a redundant branch (described previously).
 * In either case, the two blocks can be combined into a single block.
 * This eliminates the jump at the end of Bi .
 */
void BlockSimplification::CombineBlocks(const FuncPtr &F) {
  std::vector<BlockPtr> blocks;
  blocks.reserve(F->size());
  for (const auto &bb_use : *F) {
    auto block = dyn_cast<BasicBlock>(bb_use.value());
    if (block) blocks.push_back(block);
  }

  for (const auto &block : blocks) {
    auto *BB = block.get();
    if (auto jumpInst = dyn_cast<JumpInst>(BB->terminator())) {
      auto target = dyn_cast<BasicBlock>(jumpInst->target());
      // check if its successors only has one predecessor
      if (CollectPredecessors(F, target.get()).size() == 1) {
        auto target_successors = target->successors();

        // merge two blocks
        MergeBlocks(BB, target.get());

        BlockPtr merged_block = nullptr;
        for (auto &use : *F) {
          if (use.value().get() == BB) {
            merged_block = dyn_cast<BasicBlock>(use.value());
            break;
          }
        }
        DBG_ASSERT(merged_block != nullptr, "merged block not found in function");

        for (auto *succ : target_successors) {
          if (!succ) continue;
          static_cast<void>(succ->ReplacePredecessor(target.get(), merged_block));
        }

        // set it to nullptr
        std::for_each(F->begin(), F->end(), [target](Use &use) {
          if (use.value() == target) {
            use.set(nullptr);
          }
        });

        // set changed flag
        _changed = true;

        // renew the cfg
        CleanUp(F);
        return;
      }
    }
  }
}

/*
 * If Clean finds a block that contains only a jump, it can merge the block into its successor.
 * This situation arises when other passes remove all of the operations from a block Bi .
 * Consider the left graph of the pair shown in the margin.
 * Since Bi has only one successor, B j , the transformation retargets the edges that enter Bi to Bj
 * and deletes Bi from Bj’s set of predecessors.
 * This simplifies the graph. It should also speed up execution.
 * In the original graph, the paths through Bi needed two control-flow operations to reach Bj.
 * In the transformed graph, those paths use one operation to reach Bj.
 */
void BlockSimplification::RemoveEmptyBlock(const FuncPtr &F) {
  auto entry = dyn_cast<BasicBlock>(F->entry());
  std::vector<BlockPtr> blocks;
  blocks.reserve(F->size());
  for (const auto &bb_use : *F) {
    auto block = dyn_cast<BasicBlock>(bb_use.value());
    if (block) blocks.push_back(block);
  }

  for (const auto &block : blocks) {
    auto *BB = block.get();
    if (BB == entry.get()) continue;
    if (auto jumpInst = dyn_cast<JumpInst>(BB->terminator())) {
      auto target = dyn_cast<BasicBlock>(jumpInst->target());

      // check phi node
      if (IsSSA<PhiNode>(target->insts().front())) continue;

      if (BB->insts().size() == 1) {
        auto preds = CollectPredecessors(F, BB);
        if (preds.empty()) continue;

        std::unordered_set<BasicBlock *> pred_set;
        for (auto &pred : preds) {
          pred_set.insert(pred.get());
          ReplaceSuccessor(pred, target, BB);
        }

        for (auto *pred : pred_set) BB->RemovePredecessor(pred);

        // remove this block from its successor's predecessor list
        target->RemovePredecessor(BB);

        // set it to nullptr
        std::for_each(F->begin(), F->end(), [BB](Use &use) {
          if (use.value().get() == BB) {
            use.set(nullptr);
          }
        });

        // replace current block with target block
//        if (BB) BB->ReplaceBy(target);

        // set changed flag
        _changed = true;

        // renew the cfg
        CleanUp(F);
        return;
      }
    }
  }
}

void BlockSimplification::UnreachableBlockElimination(const FuncPtr &F) {
  auto entry = dyn_cast<BasicBlock>(F->entry());
  std::unordered_set<BasicBlock *> in_function;
  for (const auto &bb_use : *F) {
    auto block = dyn_cast<BasicBlock>(bb_use.value());
    if (block) in_function.insert(block.get());
  }

  std::unordered_set<BasicBlock *> reachable;
  std::vector<BlockPtr> worklist;
  worklist.push_back(entry);
  while (!worklist.empty()) {
    auto block = worklist.back();
    worklist.pop_back();
    if (!block || !reachable.insert(block.get()).second) continue;

    auto term = dyn_cast<TerminatorInst>(block->terminator());
    if (!term) continue;
    for (unsigned i = 0; i < term->GetSuccessorNum(); ++i) {
      auto succ = dyn_cast<BasicBlock>(term->GetSuccessor(i));
      if (succ && in_function.count(succ.get())) {
        worklist.push_back(succ);
      }
    }
  }

  for (auto &it : *F) {
    auto block = dyn_cast<BasicBlock>(it.value());
    if (!block) continue;
    // delete unreachable block
    if (!reachable.count(block.get())) {

      // remove the PHI value if it exists
      RemovePhiNode(block.get(), block->successors());

      it.set(nullptr);

      // set changed flag
      _changed = true;
      CleanUp(F);
      return;
    }
  }
}

void BlockSimplification::ReplaceSuccessor(BlockPtr &predecessor, BlockPtr &successor, BasicBlock *cur) {
  auto terminator = dyn_cast<TerminatorInst>(predecessor->terminator());
  DBG_ASSERT(terminator != nullptr, "predecessor block has no terminator");
  DBG_ASSERT(terminator->ReplaceSuccessor(cur, successor), "current block is not a successor");
  successor->AddPredecessor(predecessor);
}

void BlockSimplification::MergeBlocks(BasicBlock *pred, BasicBlock *succ) {
  auto jumpInst = dyn_cast<JumpInst>(pred->terminator());
  DBG_ASSERT(jumpInst != nullptr, "merged predecessor should end with jump");
  jumpInst->EraseFromParent();
  pred->AppendInstsFrom(succ);
}

void BlockSimplification::RemovePhiNode(BasicBlock *block, std::vector<BasicBlock *> successors) {
  for (auto &succ : successors) {
    for (auto inst_it = succ->insts().begin(); inst_it != succ->insts().end();) {
      auto phi_node = dyn_cast<PhiNode>(*inst_it);
      if (!phi_node) break;

      // 1. if the successor has only two predecessors, then replace the phi-node with the other value.
      // 2. if the successor has multiple predecessors, then remove the corresponding value.
      if (phi_node->size() == 2) {
        auto idx = phi_node->incomingIndexOf(block);
        DBG_ASSERT(idx >= 0, "incoming predecessor not found");
        unsigned other_idx = (static_cast<unsigned>(idx) + 1) % 2;

        // replace phi_node from its users with the other value
        phi_node->ReplaceBy((*phi_node)[other_idx].value());

        // remove phi from succ's instructions list
        inst_it = phi_node->EraseFromParent();
      } else {
        phi_node->removeIncoming(block);

        if (phi_node->size() == 1) {
          phi_node->ReplaceBy(phi_node->begin()->value());
          inst_it = phi_node->EraseFromParent();
        } else {
          inst_it++;
        }
      }

    }
  }
}

std::vector<BlockPtr> BlockSimplification::CollectPredecessors(const FuncPtr &F, BasicBlock *target) {
  std::vector<BlockPtr> preds;
  for (const auto &bb_use : *F) {
    auto pred = dyn_cast<BasicBlock>(bb_use.value());
    if (!pred || pred.get() == target) continue;
    auto term = dyn_cast<TerminatorInst>(pred->terminator());
    if (!term) continue;
    for (unsigned i = 0; i < term->GetSuccessorNum(); ++i) {
      if (term->GetSuccessor(i).get() == target) {
        preds.push_back(pred);
        break;
      }
    }
  }
  return preds;
}

void BlockSimplification::CleanUp(const FuncPtr &F) {
  // remove all useless blocks
  F->RemoveValue(nullptr);
  // move entry to the head of list
  for (std::size_t i = 0; i < F->size(); i++) {
    if ((*F)[i].value() == _entry) {
      if (i) {
        (*F)[i].set((*F)[0].value());
        (*F)[0].set(_entry);
      }
      break;
    }
  }
}

void BlockSimplification::initialize() {
  _entry = nullptr;
  _changed = false;
  _count = 0;
}

void BlockSimplification::RebuildPredecessors(const FuncPtr &F) {
  for (const auto &bb_use : *F) {
    auto block = dyn_cast<BasicBlock>(bb_use.value());
    if (!block) continue;
    block->Clear();
  }

  for (const auto &bb_use : *F) {
    auto pred = dyn_cast<BasicBlock>(bb_use.value());
    if (!pred) continue;
    for (auto *succ : pred->successors()) {
      if (!succ) continue;
      succ->AddPredecessor(pred);
    }
  }
}


static PassRegisterFactory<BlockSimplificationFactory> registry;

}
