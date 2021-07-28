#include "blkrearrange.h"
#include "common/casting.h"
#include "back/arch/arm/instdef.h"

namespace lava::back {


void BlockRearrange::runOn(const LLFunctionPtr &func) {
  if (func->is_decl()) return;
  std::vector<LLBlockPtr> blocks;

  DFS(func->blocks().front());
  _blocks.push_back(_exit);

  func->SetBlocks(_blocks);
}

void BlockRearrange::DFS(LLBlockPtr BB) {
  // return if visited
  if (!_records.insert(BB).second) return;

  if (BB->insts().back()->classId() == ClassId::LLReturnId) {
    _exit = BB;
    return;
  }

  _blocks.push_back(BB);

  auto termInst = BB->insts().back();
  if (auto jump_inst = dyn_cast<LLJump>(termInst)) {
    // visit its successor
    DFS(jump_inst->target());
  } else if (auto branch_inst = dyn_cast<LLBranch>(termInst)) {
    // visit false block firstly
    DFS(branch_inst->false_block());
    DFS(branch_inst->true_block());
  } else if (auto ret_inst = dyn_cast<LLReturn>(termInst)) {
    // do nothing
  } else {
    ERROR("should not reach here");
  }
}

}