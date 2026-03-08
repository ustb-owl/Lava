#include "opt/expression_context.h"

#include <algorithm>

#include "common/casting.h"

namespace lava::opt {

using namespace lava::mid;

bool IsPureScalarCall(const std::shared_ptr<CallInst> &call_inst,
                      const FuncInfoMap               &function_infos) {
  auto func = call_inst->Callee();
  auto it   = function_infos.find(func.get());
  if (it == function_infos.end() || !it->second.IsPure())
    return false;

  auto none_array_arg =
      std::none_of(call_inst->begin(), call_inst->end(), [](const Use &use) {
        return IsSSA<AccessInst>(use.value());
      });
  return none_array_arg;
}

bool IsPureScalarExpression(const SSAPtr      &value,
                            const FuncInfoMap &function_infos) {
  return IsSSA<BinaryOperator>(value) || IsSSA<AccessInst>(value) ||
         IsSSA<ICmpInst>(value) || IsSSA<CastInst>(value) ||
         (IsSSA<CallInst>(value) &&
          lava::opt::IsPureScalarCall(dyn_cast<CallInst>(value),
                                      function_infos));
}

void ExpressionContext::SetFunctionInfos(const FuncInfoMap &function_infos) {
  _function_infos = &function_infos;
}

void ExpressionContext::Reset() {
  _leader_cache.clear();
  _expr_table.Clear();
  _constant_ints.clear();
  _current_block = nullptr;
}

void ExpressionContext::ResetForBlock(BasicBlock *block) {
  Reset();
  _current_block = block;
}

void ExpressionContext::Forget(const SSAPtr &value) {
  _leader_cache.erase(value);
}

bool ExpressionContext::IsPureScalarCall(const SSAPtr &value) const {
  DBG_ASSERT(_function_infos != nullptr, "function info is not set");
  if (auto call_inst = dyn_cast<CallInst>(value))
    return lava::opt::IsPureScalarCall(call_inst, *_function_infos);
  return false;
}

bool ExpressionContext::IsEligibleValue(const SSAPtr &value) const {
  DBG_ASSERT(_function_infos != nullptr, "function info is not set");
  return lava::opt::IsPureScalarExpression(value, *_function_infos);
}

SSAPtr ExpressionContext::CanonicalizeConstant(
    const std::shared_ptr<ConstantInt> &constant) {
  ConstantIntKey key{CanonicalTypeId(constant->type()), constant->value()};
  auto [it, _] = _constant_ints.emplace(key, constant);
  return it->second;
}

SSAPtr ExpressionContext::Canonicalize(const SSAPtr &value) {
  DBG_ASSERT(_function_infos != nullptr, "function info is not set");
  if (auto inst = dyn_cast<Instruction>(value)) {
    if (inst->getParent() != _current_block)
      return value;
  }

  auto it = _leader_cache.find(value);
  if (it != _leader_cache.end())
    return it->second;

  SSAPtr leader = value;
  if (auto const_value = dyn_cast<ConstantInt>(value)) {
    leader = CanonicalizeConstant(const_value);
  } else if (auto key = BuildExprKey(
                 value,
                 [this](const SSAPtr &operand) {
                   return Canonicalize(operand);
                 },
                 [this](const std::shared_ptr<CallInst> &call) {
                   return lava::opt::IsPureScalarCall(call, *_function_infos);
                 })) {
    leader = _expr_table.LookupOrInsert(*key, value);
  }

  auto [res, state] = _leader_cache.emplace(value, leader);
  DBG_ASSERT(state == true, "insert new value failed");
  return res->second;
}

} // namespace lava::opt
