
#include "blocksimp.h"


int BlockMerge;

namespace lava::opt {


bool BlockSimplification::runOnFunction(const FuncPtr &F) {
  _changed = false;
  if (F->is_decl()) return _changed;

  _entry = F->entry();

  do {
    _changed = false;
    _count++;
    OnePass(F);
  } while (_changed);

  return (_count > 1);
}

void BlockSimplification::OnePass(const FuncPtr &F) {
  FoldRedundantBranch(F);
  CombineBlocks(F);
  RemoveEmptyBlock(F);
  UnreachableBlockElimination(F);
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
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto po = _blkWalker.POTraverse(entry.get());

  for (auto &BB : po) {
    if (auto branchInst = dyn_cast<BranchInst>(BB->insts().back())) {

      // get condition
      auto cond = branchInst->cond();

      // if true block == false block, then replace the branch with jump
      if (branchInst->true_block() == branchInst->false_block()) {
        // create jump instruction
        auto jump = std::make_shared<JumpInst>(branchInst->true_block());
        jump->set_logger(branchInst->logger());

        // replace the last instruction with jump
        BB->insts().back() = jump;

        // set changed flag
        _changed = true;
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
        BB->insts().erase(--(BB->insts().end()));
        BB->insts().push_back(jump_inst);

        // remove phi-node
        RemovePhiNode(BB, std::vector<BasicBlock *>{discord.get()});

        // remove current block from discord block's predecessor list
        // this block will be sweep in the following pass
        discord->RemoveValue(BB);

        // set changed flag
        _changed = true;

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
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto po = _blkWalker.POTraverse(entry.get());

  for (auto &BB : po) {
    if (auto jumpInst = dyn_cast<JumpInst>(BB->insts().back())) {
      auto target = dyn_cast<BasicBlock>(jumpInst->target());
      // check if its successors only has one predecessor
      if (target->size() == 1) {
        // merge two blocks
        MergeBlocks(BB, target.get());

        // set it to nullptr
        std::for_each(F->begin(), F->end(), [target](Use &use) {
          if (use.value() == target) {
            use.set(nullptr);
          }
        });

        // replace this block by its predecessor
        auto block = std::find_if(F->begin(), F->end(), [&BB](Use &use) { return use.value().get() == BB; });
        target->ReplaceBy((*block).value());

        // set changed flag
        _changed = true;

        // renew the cfg
        CleanUp(F);
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
  auto po = _blkWalker.POTraverse(entry.get());

  for (auto &BB : po) {
    if (BB == entry.get()) continue;
    if (auto jumpInst = dyn_cast<JumpInst>(BB->insts().back())) {
      auto target = dyn_cast<BasicBlock>(jumpInst->target());

      // check phi node
      if (IsSSA<PhiNode>(target->insts().front())) continue;

      if (BB->insts().size() == 1) {
        std::unordered_set<SSAPtr> preds;
        for (auto it = BB->begin(); it != BB->end(); it++) {
          auto pred = dyn_cast<BasicBlock>(it->value());
          preds.insert(pred);
          ReplaceSuccessor(pred, target, BB);
        }

        for (auto &it : preds) BB->RemoveValue(it);

        // remove this block from its successor's predecessor list
        target->RemoveValue(BB);

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
      }
    }
  }
}

void BlockSimplification::UnreachableBlockElimination(const FuncPtr &F) {
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto po = _blkWalker.RPOTraverse(entry.get());
  for (auto &it : *F) {
    auto block = dyn_cast<BasicBlock>(it.value());
    if (!block) continue;
    // delete unreachable block
    auto res = std::find(po.begin(), po.end(), block.get());
    if (res == po.end()) {

      // remove the PHI value if it exists
      RemovePhiNode(block.get(), block->successors());

      it.set(nullptr);
      block->RemoveFromUser();

      // set changed flag
      _changed = true;
    }
  }

  // renew the cfg
  CleanUp(F);
}

void BlockSimplification::ReplaceSuccessor(BlockPtr &predecessor, BlockPtr &successor, BasicBlock *cur) {
  if (auto jumpInst = dyn_cast<JumpInst>(predecessor->insts().back())) {
    jumpInst->RemoveValue(cur);
    jumpInst->AddValue(successor);
  } else if (auto branchInst = dyn_cast<BranchInst>(predecessor->insts().back())) {
    auto true_block = branchInst->true_block();
    auto false_block = branchInst->false_block();
    if (cur == true_block.get()) {
      branchInst->SetTrueBlock(successor);
    } else if (cur == false_block.get()) {
      branchInst->SetFalseBlock(successor);
    } else {
      ERROR("should not reach here");
    }
  }

  // add predecessor into target's predecessor list
  for (const auto &pred : (*successor)) {
    if (pred.value() == predecessor) return;
  }
  successor->AddValue(predecessor);
}

void BlockSimplification::MergeBlocks(BasicBlock *pred, BasicBlock *succ) {
  auto &insts = pred->insts();

  // remove jump instruction
  auto back = insts.back();
  auto jumpInst = dyn_cast<JumpInst>(back);
  jumpInst->RemoveValue(succ);
  insts.pop_back();

  // move successor's instructions into predecessor
  insts.insert(pred->inst_end(), succ->inst_begin(), succ->inst_end());
}

void BlockSimplification::RemovePhiNode(BasicBlock *block, std::vector<BasicBlock *> successors) {
  for (auto &succ : successors) {
    for (auto inst_it = succ->insts().begin(); inst_it != succ->insts().end();) {
      auto phi_node = dyn_cast<PhiNode>(*inst_it);
      if (!phi_node) break;

      // 1. if the successor has only two predecessors, then replace the phi-node with the other value.
      // 2. if the successor has multiple predecessors, then remove the corresponding value.
      if (phi_node->size() == 2) {
        unsigned idx;
        for (idx = 0; idx != 2; idx++) {
          if (phi_node->getIncomingBlock(idx).get() == block) {
            unsigned other_idx = (idx + 1) % 2;

            // replace phi_node from its users with the other value
            phi_node->ReplaceBy((*phi_node)[other_idx].value());

            // clear phi_node's operands
            phi_node->Clear();

            // remove phi from succ's instructions list
            inst_it = succ->insts().erase(inst_it);
            break;
          }
        }
      } else {
        unsigned idx;
        for (idx = 0; idx != phi_node->size(); idx++) {
          if (phi_node->getIncomingBlock(idx).get() == block) {
            phi_node->RemoveValue(idx);
            break;
          }
        }

        if (phi_node->size() == 1) {
          phi_node->ReplaceBy(phi_node->begin()->value());
          phi_node->Clear();
          inst_it = succ->insts().erase(inst_it);
        } else {
          inst_it++;
        }
      }

    }
  }
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


static PassRegisterFactory<BlockSimplificationFactory> registry;

}