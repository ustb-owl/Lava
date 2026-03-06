#include "opt/transforms/strength_reduction.h"

#include <bit>

#include "common/casting.h"

int StrengthReductionPass;

namespace {

using namespace lava::mid;

bool IsPositivePowerOfTwo(int value) {
  return value > 0 && std::has_single_bit(static_cast<unsigned>(value));
}

InstPtr ReduceSignedDivByPowerOfTwo(const std::shared_ptr<BinaryOperator> &binary_inst,
                                    BasicBlock *block,
                                    InstList::iterator pos) {
  auto rhs = lava::dyn_cast<ConstantInt>(binary_inst->RHS());
  if (rhs == nullptr || binary_inst->opcode() != BinaryOperator::BinaryOps::SDiv) return nullptr;
  if (rhs->value() <= 1 || !IsPositivePowerOfTwo(rhs->value())) return nullptr;

  auto type = binary_inst->type();
  auto make_const = [&type](int value) {
    auto constant = std::make_shared<ConstantInt>(value);
    constant->set_type(type);
    return constant;
  };

  auto divisor = static_cast<unsigned>(rhs->value());
  auto shift_amount = static_cast<int>(std::countr_zero(divisor));
  auto sign_shift = static_cast<int>(type->GetSize() - 1);
  auto bias_mask = static_cast<int>(divisor - 1);

  auto sign = BinaryOperator::Create(Instruction::AShr, binary_inst->LHS(), make_const(sign_shift));
  auto bias = BinaryOperator::Create(Instruction::And, sign, make_const(bias_mask));
  auto adjusted = BinaryOperator::Create(Instruction::Add, binary_inst->LHS(), bias);
  auto reduced = BinaryOperator::Create(Instruction::AShr, adjusted, make_const(shift_amount));

  block->InsertInst(pos, sign);
  block->InsertInst(pos, bias);
  block->InsertInst(pos, adjusted);
  block->InsertInst(pos, reduced);
  return reduced;
}

}

namespace lava::opt {

bool StrengthReduction::runOnFunction(const FuncPtr &F) {
  _changed = false;
  if (F->is_decl()) return false;

  for (const auto &BB : *F) {
    auto block = dyn_cast<BasicBlock>(BB.value());
    for (auto it = block->inst_begin(); it != block->inst_end();) {
      auto next = std::next(it);
      if (auto binary_inst = dyn_cast<BinaryOperator>(*it)) {
        if (auto reduced = ReduceSignedDivByPowerOfTwo(binary_inst, block.get(), it)) {
          binary_inst->ReplaceBy(reduced);
          _changed = true;
        }
      }
      it = next;
    }
  }

  return _changed;
}

static PassRegisterFactory<StrengthReductionFactory> registry;

}
