#include "opt/transforms/inst_simplify.h"

#include "common/casting.h"
#include "opt/register.h"

int InstSimplifyPass;

namespace {

bool IncomingValuesSame(const lava::mid::SSAPtr &lhs,
                        const lava::mid::SSAPtr &rhs) {
  if (lhs == rhs)
    return true;
  auto lhs_const = lava::dyn_cast<lava::mid::ConstantInt>(lhs);
  auto rhs_const = lava::dyn_cast<lava::mid::ConstantInt>(rhs);
  if (lhs_const && rhs_const) {
    return lhs_const->value() == rhs_const->value() &&
           lhs_const->type()->IsIdentical(rhs_const->type());
  }
  return false;
}

} // namespace

namespace lava::opt {

bool InstSimplify::ReplaceAndEraseIfChanged(const InstPtr &inst,
                                            const SSAPtr  &value) {
  if (value == nullptr || value == inst)
    return false;
  inst->ReplaceBy(value);
  inst->EraseFromParent();
  _changed = true;
  return true;
}

bool InstSimplify::SimplifyBinary(
    const std::shared_ptr<BinaryOperator> &binary_inst) {
  bool changed = false;

  if (binary_inst->LHS()->classId() == ClassId::ConstantIntId) {
    changed |= binary_inst->swapOperand();
  }

  auto lhs_const = dyn_cast<ConstantInt>(binary_inst->LHS());
  auto rhs_const = dyn_cast<ConstantInt>(binary_inst->RHS());
  if (lhs_const != nullptr && rhs_const != nullptr) {
    return ReplaceAndEraseIfChanged(binary_inst,
                                    binary_inst->EvalArithOnConst()) ||
           changed;
  }

  changed |= binary_inst->TryToFold();
  if (ReplaceAndEraseIfChanged(binary_inst, binary_inst->OptimizedValue()))
    return true;

  if (changed)
    _changed = true;
  return changed;
}

bool InstSimplify::SimplifyCmp(const std::shared_ptr<ICmpInst> &icmp_inst) {
  auto lhs_const = dyn_cast<ConstantInt>(icmp_inst->LHS());
  auto rhs_const = dyn_cast<ConstantInt>(icmp_inst->RHS());
  if (lhs_const == nullptr || rhs_const == nullptr)
    return false;
  return ReplaceAndEraseIfChanged(icmp_inst, icmp_inst->EvalArithOnConst());
}

bool InstSimplify::SimplifyPhi(const std::shared_ptr<PhiNode> &phi_node) {
  if (phi_node->size() == 0)
    return false;

  auto first    = phi_node->getIncomingValueAt(0);
  bool all_same = true;
  auto size     = phi_node->size();
  for (std::size_t i = 1; i < size && all_same; ++i) {
    all_same &= IncomingValuesSame(first, phi_node->getIncomingValueAt(i));
  }

  if (!all_same)
    return false;
  return ReplaceAndEraseIfChanged(phi_node, first);
}

bool InstSimplify::runOnFunction(const FuncPtr &F) {
  if (F->is_decl())
    return false;

  bool changed_any = false;
  bool changed     = false;
  do {
    changed  = false;
    _changed = false;
    for (const auto &BB : *F) {
      for (auto it = BB->inst_begin(); it != BB->inst_end();) {
        auto inst = *it;
        it        = std::next(it);
        if (auto binary_inst = dyn_cast<BinaryOperator>(inst)) {
          changed |= SimplifyBinary(binary_inst);
        } else if (auto phi_node = dyn_cast<PhiNode>(inst)) {
          changed |= SimplifyPhi(phi_node);
        } else if (auto icmp_inst = dyn_cast<ICmpInst>(inst)) {
          changed |= SimplifyCmp(icmp_inst);
        }
      }
    }
    changed_any |= changed;
  } while (changed);

  return changed_any;
}

void RegisterInstSimplifyPass() {
  RegisterPassCliMetadata({
      "InstSimplify",
      "inst-simplify",
      {"simplify"},
      "perform local constant folding and algebraic cleanup",
  });
  static PassRegisterFactory<InstSimplifyFactory> registry;
}

} // namespace lava::opt
