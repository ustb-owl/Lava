#include "funcfix.h"
#include "common/casting.h"

namespace lava::back {


void FunctionFix::runOn(const LLFunctionPtr &func) {
  for (const auto &block : func->blocks()) {
    for (auto it = block->inst_begin(); it != block->inst_end(); it++) {
      if ((*it)->opcode() == LLInst::Opcode::Return) {
        _ret_pos = it;
        _ret_block = block;
      }
    }
  }

  for (const auto &it : func->saved_regs()) _saved_regs.push_back(it);

  // save lr if has call instruction
  if (func->has_call_inst()) _saved_regs.push_back(ArmReg::lr);

  // TODO: dirty hack, save fp for now
//  _saved_regs.push_back(ArmReg::fp);

  // sort regs
  std::sort(_saved_regs.begin(), _saved_regs.end());

  // get offset
  _gap = 4 * _saved_regs.size() + func->stack_size();

  AddPrologue(func);
  AddEpilogue(func);
}

/* save registers, update fp, renew argument offset
 * push { rn, ..., fp }
 * mov fp, sp
 * add sp, fp, stack_size
 */
void FunctionFix::AddPrologue(const LLFunctionPtr &func) {
  auto entry = func->entry();
  DBG_ASSERT(entry->name() == "entry", "not entry");

  _module.SetInsertPoint(entry, entry->inst_begin());
  if (!_saved_regs.empty()) {
    auto push_inst = _module.AddInst<LLPush>(_saved_regs);
  }


  LLOperandPtr stack_size;
  if (_module.can_encode_imm(func->stack_size())) {
    stack_size = _module.CreateImmediate(func->stack_size());
  } else {
    auto dst = LLOperand::Register(ArmReg::r12);
    _module.AddInst<LLLoadPseudo>(dst, func->stack_size());
    stack_size = dst;
  }
  DBG_ASSERT(stack_size != nullptr, "create stack size failed");

  auto update_sp = _module.AddInst<LLBinaryInst>(LLInst::Opcode::Sub,
                                                 LLOperand::Register(ArmReg::sp),
                                                 LLOperand::Register(ArmReg::sp), stack_size);

  DBG_ASSERT(update_sp != nullptr, "update sp register failed");

  for (const auto &inst : entry->insts()) {
    if (auto move_inst = dyn_cast<LLMove>(inst)) {
      if (!move_inst->is_arg()) continue;

      auto src = move_inst->src();
      DBG_ASSERT(src->IsImmediate(), "offset is not imm");
      auto offset = src->imm_num();

      offset += _gap;
      move_inst->set_operand(LLOperand::Immediate(offset), 0);
    }
  }
}

void FunctionFix::AddEpilogue(const LLFunctionPtr &func) {
  _module.SetInsertPoint(_ret_block, _ret_pos);

  LLOperandPtr stack_size;
  if (_module.can_encode_imm(func->stack_size())) {
    stack_size = _module.CreateImmediate(func->stack_size());
  } else {
    auto dst = LLOperand::Register(ArmReg::r12);
    _module.AddInst<LLComment>("split stack size: " + std::to_string(func->stack_size()));
    _module.AddInst<LLLoadPseudo>(dst, func->stack_size());
    stack_size = dst;
  }
  DBG_ASSERT(stack_size != nullptr, "create stack size failed");

  auto update_sp = _module.AddInst<LLBinaryInst>(LLInst::Opcode::Add,
                                                 LLOperand::Register(ArmReg::sp),
                                                 LLOperand::Register(ArmReg::sp), stack_size);

  DBG_ASSERT(update_sp != nullptr, "update sp register failed");

  if (!_saved_regs.empty()) {
    auto pop_inst = _module.AddInst<LLPop>(_saved_regs);
    DBG_ASSERT(pop_inst != nullptr, "create pop instruction failed");
  }

}


}