#include "tailrecursion.h"
#include "common/casting.h"

namespace lava::back {

/*
 int f() {
    ...
    return f();
    ...
    return 1;
 }
 To:
 int f() {
    entry:
    ...
    goto entry;
    ...
    return 1;
 */
void TailRecursionTransform::runOn(const LLFunctionPtr &func) {
  if (!func->is_tail_recursion()) return;

  std::string func_name = func->function()->GetFunctionName();
  auto pre_head = std::make_shared<LLBasicBlock>(func_name + "_pre_head", nullptr, func);
  auto head = func->blocks().begin();
  auto entry = func->entry();
  entry->SetBlockName(func_name + "_entry");
  auto jump_to_entry = std::make_shared<LLJump>(entry);
  pre_head->insts().push_back(jump_to_entry);
  func->blocks().insert(head, pre_head);

  for (const auto &block : func->blocks()) {
    for (auto it = block->insts().begin(); it != block->insts().end(); it++) {
      if (auto call_inst = dyn_cast<LLCall>(*it)) {
        if (call_inst->IsTailCall()) {
          if (call_inst->function() == func->function()) {
            auto jump_inst = std::make_shared<LLJump>(entry);
            block->insts().insert(it, jump_inst);
            while (it != block->insts().end()) it = block->insts().erase(it);
          }
        }
      }
    }
  }

}

}