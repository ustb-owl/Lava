#include <algorithm>

#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int BlockMerge;

namespace lava::opt {

/*
 reference from <Engineering a compiler> 10.2.2
 simplify the structure of basic blocks
*/
class BlockSimplification : public FunctionPass {
private:
  bool _changed;
  SSAPtr _entry;
  BlockWalker _blkWalker;

public:
  bool runOnFunction(const FuncPtr &F) final {
    if (F->is_decl()) return _changed;

    _entry = F->entry();

    do {
      _changed = false;
      OnePass(F);
    } while (_changed);

    return _changed;
  }

  void OnePass(const FuncPtr &F) {
    FoldRedundantBranch(F);
    CombineBlocks(F);
    RemoveEmptyBlock(F);
    UnreachableBlockElimination(F);
  }

  /* 1. Fold a Redundant Branch
   * If Clean finds a block that ends in a branch, and both sides of the branch target the same block,
   * it replaces the branch with a jump to the target block.
   * This situation arises as the result of other simplifications.
   * For example, Bi might have had two successors, each with a jump to B j .
   * If another transformation had already emptied those blocks, then empty-block removal,
   * discussed next, might produce the initial graph shown in the margin.
   */
  void FoldRedundantBranch(const FuncPtr &F) {
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
        } else if (cond->IsConst()) {
          BlockPtr target = nullptr, discord = nullptr;
          // check if condition is true
          if (dyn_cast<ConstantInt>(cond)->IsZero()) {
            target = dyn_cast<BasicBlock>(branchInst->false_block());
            discord = dyn_cast<BasicBlock>(branchInst->true_block());
          } else {
            target = dyn_cast<BasicBlock>(branchInst->true_block());
            discord = dyn_cast<BasicBlock>(branchInst->false_block());
          }

          // remove current block from discord block's predecessor list
          // this block will be sweep in the following pass
          discord->RemoveValue(BB);

          // set changed flag
          _changed = true;

        }
      }
    }
  }

  /* 2. Combine blocks
   * If Clean finds a block Bi that ends in a jump to B j and B j has only one predecessor,
   * it can combine the two blocks, as shown in the margin.
   * This situation can arise in several ways.
   * Another transformation might eliminate other edges that entered B j , or Bi and
   * B j might be the result of folding a redundant branch (described previously).
   * In either case, the two blocks can be combined into a single block.
   * This eliminates the jump at the end of Bi .
   */
  void CombineBlocks(const FuncPtr &F) {
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

  /* 3. Remove an Empty Block
   * If Clean finds a block that contains only a jump, it can merge the block into its successor.
   * This situation arises when other passes remove all of the operations from a block Bi .
   * Consider the left graph of the pair shown in the margin.
   * Since Bi has only one successor, B j , the transformation retargets the edges that enter Bi to Bj
   * and deletes Bi from Bj’s set of predecessors.
   * This simplifies the graph. It should also speed up execution.
   * In the original graph, the paths through Bi needed two control-flow operations to reach Bj.
   * In the transformed graph, those paths use one operation to reach Bj.
   */
  void RemoveEmptyBlock(const FuncPtr &F) {
    auto entry = dyn_cast<BasicBlock>(F->entry());
    auto po = _blkWalker.POTraverse(entry.get());

    for (auto &BB : po) {
      if (auto jumpInst = dyn_cast<JumpInst>(BB->insts().back())) {
        auto target = dyn_cast<BasicBlock>(jumpInst->target());
        if (BB->insts().size() == 1) {
          for (auto &it : *BB) {
            auto pred = dyn_cast<BasicBlock>(it.data());
            ReplaceSuccessor(pred, target, BB);
          }

          // remove this block from its successor's predecessor list
          target->RemoveValue(BB);

          // set it to nullptr
          std::for_each(F->begin(), F->end(), [BB](Use &use) {
            if (use.value().get() == BB) {
              use.set(nullptr);
            }
          });

          // replace current block with target block
          BB->ReplaceBy(target);

          // set changed flag
          _changed = true;

          // renew the cfg
          CleanUp(F);
        }
      }
    }
  }

  void UnreachableBlockElimination(const FuncPtr &F) {
    for (auto &it : *F) {
      auto block = dyn_cast<BasicBlock>(it.value());
      if (!block) continue;
      // delete unreachable block
      if (block->empty() && block != _entry) {
        it.set(nullptr);
        block->RemoveFromUser();

        // set changed flag
        _changed = true;
      }
    }

    // renew the cfg
    CleanUp(F);
  }

  void ReplaceSuccessor(BlockPtr &predecessor, BlockPtr &successor, BasicBlock *cur) {
    auto termInst = predecessor->insts().back();

    // add predecessor into target's predecessor list
    successor->AddValue(predecessor);

    // pred from BB's predecessor list
    cur->RemoveValue(predecessor);
  }

  void MergeBlocks(BasicBlock *pred, BasicBlock *succ) {
    auto &insts = pred->insts();

    // remove jump instruction
    auto back = insts.back();
    auto jumpInst = dyn_cast<JumpInst>(back);
    jumpInst->RemoveValue(succ);
    insts.pop_back();

    // move successor's instructions into predecessor
    insts.insert(pred->inst_end(), succ->inst_begin(), succ->inst_end());
  }

  void CleanUp(const FuncPtr &F) {
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

  void initialize() final {
    _entry = nullptr;
    _changed = false;
  }
};

class BlockSimplificationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<BlockSimplification>();
    auto passinfo = std::make_shared<PassInfo>(pass, "BlockSimplification", false, false, BLOCK_SIMPLIFICATION);
    return passinfo;
  }
};

static PassRegisterFactory<BlockSimplificationFactory> registry;

}