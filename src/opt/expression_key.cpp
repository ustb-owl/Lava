#include "opt/expression_key.h"

#include <algorithm>
#include <functional>

#include "common/casting.h"

namespace lava::opt {

using namespace lava::mid;

namespace {

bool IsCommutative(BinaryOperator::BinaryOps opcode) {
  switch (opcode) {
  case BinaryOperator::BinaryOps::Add:
  case BinaryOperator::BinaryOps::Mul:
  case BinaryOperator::BinaryOps::And:
  case BinaryOperator::BinaryOps::Or:
    return true;
  default:
    return false;
  }
}

std::optional<front::Operator> ReversedPredicate(front::Operator op) {
  using Operator = front::Operator;
  switch (op) {
  case Operator::SLess:
    return Operator::SGreat;
  case Operator::SGreat:
    return Operator::SLess;
  case Operator::SGreatEq:
    return Operator::SLessEq;
  case Operator::SLessEq:
    return Operator::SGreatEq;
  default:
    return std::nullopt;
  }
}

bool ShouldSwapOperands(const SSAPtr &lhs, const SSAPtr &rhs) {
  return std::less<const Value *>{}(rhs.get(), lhs.get());
}

std::optional<ExprKey>
BuildExprKeyForBinary(const std::shared_ptr<BinaryOperator> &binary_inst,
                      const LeaderLookup                    &leader_lookup) {
  auto lhs = leader_lookup(binary_inst->LHS());
  auto rhs = leader_lookup(binary_inst->RHS());
  if (IsCommutative(binary_inst->opcode()) && ShouldSwapOperands(lhs, rhs))
    std::swap(lhs, rhs);

  ExprKey key;
  key.kind           = ExprKind::Binary;
  key.opcode         = binary_inst->opcode();
  key.result_type_id = CanonicalTypeId(binary_inst->type());
  key.operands       = {lhs.get(), rhs.get()};
  return key;
}

std::optional<ExprKey>
BuildExprKeyForAccess(const std::shared_ptr<AccessInst> &access_inst,
                      const LeaderLookup                &leader_lookup) {
  ExprKey key;
  key.kind           = ExprKind::Access;
  key.extra          = static_cast<int>(access_inst->acc_type());
  key.result_type_id = CanonicalTypeId(access_inst->type());
  key.operands.reserve(access_inst->size());
  for (unsigned i = 0; i < access_inst->size(); ++i) {
    key.operands.push_back(leader_lookup((*access_inst)[i].value()).get());
  }
  return key;
}

std::optional<ExprKey>
BuildExprKeyForCall(const std::shared_ptr<CallInst> &call_inst,
                    const LeaderLookup              &leader_lookup,
                    const PureCallPredicate         &is_pure_call) {
  if (!is_pure_call(call_inst))
    return std::nullopt;

  ExprKey key;
  key.kind           = ExprKind::Call;
  key.result_type_id = CanonicalTypeId(call_inst->type());
  key.symbol         = call_inst->Callee().get();
  key.operands.reserve(call_inst->param_size());
  for (int i = 0; i < call_inst->param_size(); ++i) {
    key.operands.push_back(leader_lookup(call_inst->Param(i)).get());
  }
  return key;
}

std::optional<ExprKey>
BuildExprKeyForICmp(const std::shared_ptr<ICmpInst> &icmp_inst,
                    const LeaderLookup              &leader_lookup) {
  auto op  = icmp_inst->op();
  auto lhs = leader_lookup(icmp_inst->LHS());
  auto rhs = leader_lookup(icmp_inst->RHS());
  auto rev = ReversedPredicate(op);
  if (rev.has_value() && ShouldSwapOperands(lhs, rhs)) {
    std::swap(lhs, rhs);
    op = *rev;
  }

  ExprKey key;
  key.kind           = ExprKind::ICmp;
  key.extra          = static_cast<int>(op);
  key.result_type_id = CanonicalTypeId(icmp_inst->type());
  key.operands       = {lhs.get(), rhs.get()};
  return key;
}

std::optional<ExprKey>
BuildExprKeyForCast(const std::shared_ptr<CastInst> &cast_inst,
                    const LeaderLookup              &leader_lookup) {
  ExprKey key;
  key.kind           = ExprKind::Cast;
  key.opcode         = cast_inst->opcode();
  key.result_type_id = CanonicalTypeId(cast_inst->type());
  key.operands       = {leader_lookup(cast_inst->operand()).get()};
  return key;
}

} // namespace

std::string CanonicalTypeId(const define::TypePtr &type) {
  return type == nullptr ? "" : type->GetTrivialType()->GetTypeId();
}

std::optional<ExprKey> BuildExprKey(const SSAPtr            &value,
                                    const LeaderLookup      &leader_lookup,
                                    const PureCallPredicate &is_pure_call) {
  if (auto binary_inst = dyn_cast<BinaryOperator>(value))
    return BuildExprKeyForBinary(binary_inst, leader_lookup);
  if (auto access_inst = dyn_cast<AccessInst>(value))
    return BuildExprKeyForAccess(access_inst, leader_lookup);
  if (auto call_inst = dyn_cast<CallInst>(value))
    return BuildExprKeyForCall(call_inst, leader_lookup, is_pure_call);
  if (auto icmp_inst = dyn_cast<ICmpInst>(value))
    return BuildExprKeyForICmp(icmp_inst, leader_lookup);
  if (auto cast_inst = dyn_cast<CastInst>(value))
    return BuildExprKeyForCast(cast_inst, leader_lookup);
  return std::nullopt;
}

} // namespace lava::opt
