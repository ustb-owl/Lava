#include "spill.h"
#include "common/casting.h"

namespace lava::back {


void Spill::runOn(const LLFunctionPtr &func) {
  if (func->is_decl()) return;

  for (const auto &block : func->blocks()) {
    _cur_block = block;
    for (auto it = block->inst_begin(); it != block->inst_end(); it++) {
      auto inst = *it;
      auto mask = GetTmpMask(inst);
      if (auto mv_inst = dyn_cast<LLMove>(inst)) {
        if (mv_inst->src()->IsVirtual()) {
          auto allocated = mv_inst->src()->allocated();
          if (allocated->IsRealReg()) {
            mv_inst->set_operand(allocated, 0);
          } else {
            // insert load to dest and remove current move
            InsertLoadInst(it, allocated, mv_inst->dst());
            it = --(block->insts().erase(it));
            inst = *it;
          }
        }

      } else if (inst->classId() != ClassId::LLMoveId) {

        /* ------ spill operands ------*/
        // used for replace operand
        std::size_t index = 0;
        for (const auto &opr : inst->operands()) {
          // continue if operand is nullptr or operand is not virtual register
          if (opr == nullptr || !opr->IsVirtual()) {
            index++;
            continue;
          }

          auto allocated_value = opr->allocated();

          // update dst
          if (allocated_value->IsRealReg()) {
            inst->set_operand(allocated_value, index);
          } else {
            auto dst = GetTmpReg(mask);
            InsertLoadInst(it, opr->allocated(), dst);
            inst->set_operand(dst, index);
          }

          // update index
          index++;
        }

      }

      /* ------ spill dst ------ */
      auto dst = inst->dest();
      if ((dst != nullptr) && dst->IsVirtual()) {
        auto tmp = GetTmpReg(mask);
        InsertStoreInst(it, dst->allocated(), tmp);
        inst->set_dst(tmp);
      }
    }
  }
}

LLOperandPtr Spill::GetTmpReg(std::uint32_t &reg_mask) {
  LLOperandPtr temp;
  // try to use 'r12' first
  if (!(reg_mask & (1 << static_cast<int>(ArmReg::r12)))) {
    reg_mask |= 1 << static_cast<int>(ArmReg::r12);
    temp = LLOperand::Register(ArmReg::r12);
  } else if (!(reg_mask & (1 << static_cast<int>(ArmReg::r10)))) {
    reg_mask |= 1 << static_cast<int>(ArmReg::r10);
    temp = LLOperand::Register(ArmReg::r10);
  }
  DBG_ASSERT(temp != nullptr, "get tmp register(r10/r12) failed");
  return temp;
}

std::uint32_t Spill::GetTmpMask(const LLInstPtr &inst) {
  std::uint32_t mask = 0;
  auto ptr = dyn_cast<LLInst>(inst);
  DBG_ASSERT(ptr != nullptr, "get LLInst failed");
  for (const auto &opr : ptr->operands()) {
    if (opr == nullptr) continue;
    if (opr->IsRealReg() && !opr->IsVirtual()) {
      auto name = static_cast<ArmReg>(opr->reg());
      mask |= 1 << static_cast<int>(name);
    }
  }
  return mask;
}

void Spill::InsertLoadInst(LLInstList::iterator &it,
                           const LLOperandPtr &slot, const LLOperandPtr &dst) {

  // set insert point
  _module.SetInsertPoint(_cur_block, it);
  _module.AddInst<LLComment>("load virtual register from memory");

  // create operand
  DBG_ASSERT(slot->IsImmediate(), "slot offset is not immediate number");
  auto fp = LLOperand::Register(ArmReg::fp);

  // create load instruction
  auto load_inst = _module.AddInst<LLLoad>(dst, fp, slot);
  DBG_ASSERT(load_inst, "create load instruction failed");

//  it = _module.InsertPos();
}

void Spill::InsertStoreInst(LLInstList::iterator &it,
                            const LLOperandPtr &slot, const LLOperandPtr &tmp) {

  // set insert point
  _module.SetInsertPoint(_cur_block, ++it);
  _module.AddInst<LLComment>("store virtual register into memory");

  // create operand
  DBG_ASSERT(slot->IsImmediate(), "slot offset is not immediate number");
  auto fp = LLOperand::Register(ArmReg::fp);

  auto store_inst = _module.AddInst<LLStore>(tmp, fp, slot);
  DBG_ASSERT(store_inst, "create store instruction failed");

  it = --_module.InsertPos();
}

}