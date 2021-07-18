#include "slot.h"

namespace lava::back {

LLOperandPtr SlotAllocator::AllocSlot(const LLFunctionPtr &func, const LLOperandPtr &operand) {
  DBG_ASSERT(operand->state() == LLOperand::State::Virtual, "only virtual register can be spill to stack");

  auto res = func->stack_size();
  func->SetStackSize(res + 4);

  // record the newest stack size
  if (_slots.find(func) == _slots.end()) {
    _slots.insert({func, func->stack_size()});
  } else {
    _slots[func] = func->stack_size();
  }

  // set offset
  auto offset = LLOperand::Immediate(-res);
  operand->set_allocated(offset);

  return offset;
}

}
