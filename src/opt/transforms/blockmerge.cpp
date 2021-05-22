#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "mid/ir/castssa.h"
#include "opt/pass_manager.h"

int BlockMerge;

namespace lava::opt {

/*
  merge blocks that connected with jump instructions

  e.g.
    %0:
      ...                         %0:
      jump %1                       ...
    %1: ; preds: %0                 ...
      ...                 ==>>      jump %2
      jump %2                     %2: ; preds: %0, %3
    %2: ; preds: %1, %3             ...
      ...                           jump %4
      jump %4

*/
class BlockMerge : public FunctionPass {
private:
  bool _changed;

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    SSAPtr entry;
    for (auto &it : *F) {
      if (entry == nullptr) entry = it.get();
      auto block = CastTo<BasicBlock>(it.get());

      // check if this block has only one predecessor
      if (block->size() == 1) {
        auto pred = CastTo<BasicBlock>((*block)[0].get());
        auto inst = *(--pred->inst_end());
        auto branch_inst = CastTo<Instruction>(inst);

        // check if its predecessor has only one successor
        if (branch_inst->opcode() == Instruction::TermOps::Ret ||
            branch_inst->opcode() == Instruction::TermOps::Jmp) {
          _changed = true;

          // merge two blocks
          MergeBlocks(pred, block);
          block->ReplaceBy(pred);
          it.set(nullptr);
        }

      }

      // delete unreachable block
      if (block->empty() && block != entry) {
        it.set(nullptr);
        block->RemoveFromUser();
      }
    }

    // remove all useless blocks
    F->RemoveValue(nullptr);
    // move entry to the head of list
    for (std::size_t i = 0; i < F->size(); i++) {
      if ((*F)[i].get() == entry) {
        if (i) {
          (*F)[i].set((*F)[0].get());
          (*F)[0].set(entry);
        }
        break;
      }
    }

    return _changed;
  }

  void MergeBlocks(const BlockPtr &pred, const BlockPtr &succ) {
    auto &insts = pred->insts();

    // remove jump instruction
    insts.pop_back();

    // move successor's instructions into predecessor
    insts.insert(pred->inst_end(), succ->inst_begin(), succ->inst_end());
  }
};

class BlockMergeFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<BlockMerge>();
    auto passinfo =  std::make_shared<PassInfo>(pass, "BlockMerge", false, false);
    return passinfo;
  }
};

static PassRegisterFactory<BlockMergeFactory> registry;

}