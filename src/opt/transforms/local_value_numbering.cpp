#include "local_value_numbering.h"

#include "opt/register.h"

int LocalValueNumberingPass;

namespace lava::opt {

const FuncInfoMap &LocalValueNumbering::FunctionInfos() const {
  return PassManager::GetAnalysisResult<FuncInfoMap>("FunctionInfoPass");
}

bool LocalValueNumbering::runOnFunction(const FuncPtr &F) {
  if (F->is_decl())
    return false;

  bool changed_any = false;
  bool changed     = false;
  do {
    initialize();

    _changed  = false;
    RunLocalValueNumbering(F);

    // Clear value numbering state before cleanup passes.
    _value_number.clear();

    // Run cleanup passes after local value numbering/CSE.
    _changed |= PassManager::RunPassOnFunction("DeadCodeElimination", F);

    _changed |= PassManager::RunPassOnFunction("BlockSimplification", F);
    changed = _changed;
    changed_any |= changed;
  } while (changed);

  return changed_any;
}

void LocalValueNumbering::initialize() {
  _cur_block = nullptr;
  static_cast<void>(
      PassManager::RequireAnalysisResult<FuncInfoMap>("FunctionInfoPass"));
}

void LocalValueNumbering::finalize() {
  _value_number.clear();
  _cur_block = nullptr;
}

void LocalValueNumbering::Replace(const InstPtr &inst, const SSAPtr &value) {
  if (inst != value) {
    inst->ReplaceBy(value);
    _changed = true;

    auto res = std::find_if(_value_number.begin(), _value_number.end(),
                            [inst](const std::pair<SSAPtr, SSAPtr> &kv) {
                              return kv.first == inst;
                            });
    if (res != _value_number.end()) {
      _value_number.erase(res);
    }
  }
}

SSAPtr LocalValueNumbering::FindValue(
    const std::shared_ptr<BinaryOperator> &binary_inst) {
  BinaryOperator::BinaryOps opcode = binary_inst->opcode();
  SSAPtr                    lhs    = ValueOf(binary_inst->LHS());
  SSAPtr                    rhs    = ValueOf(binary_inst->RHS());

  for (auto [k, v] : _value_number) {
    auto bin_value = dyn_cast<BinaryOperator>(k);

    if (bin_value && (bin_value != binary_inst)) {
      BinaryOperator::BinaryOps opcode2 = bin_value->opcode();
      if (opcode != opcode2)
        continue;
      SSAPtr lhs2 = ValueOf(bin_value->LHS());
      SSAPtr rhs2 = ValueOf(bin_value->RHS());

      bool same = false;
      if (lhs == lhs2 && rhs == rhs2)
        same = true;
      else if (lhs == rhs2 && rhs == lhs2) {
        if (opcode == BinaryOperator::BinaryOps::Add ||
            opcode == BinaryOperator::BinaryOps::Mul ||
            opcode == BinaryOperator::BinaryOps::And ||
            opcode == BinaryOperator::BinaryOps::Or) {
          same = true;
        }
      }

      if (same)
        return v;
    }
  }
  return binary_inst;
}

SSAPtr LocalValueNumbering::FindValue(
    const std::shared_ptr<AccessInst> &access_inst) {
  for (auto [k, v] : _value_number) {
    //    auto [k, v] = _value_number[i];
    auto gep = dyn_cast<AccessInst>(k);
    if (gep && (gep != access_inst)) {
      bool same = false;
      if (ValueOf(access_inst->ptr()) == ValueOf(gep->ptr())) {
        if (ValueOf(access_inst->index()) == ValueOf(gep->index())) {
          if (access_inst->size() == gep->size())
            same = true;
        }
      }
      if (same)
        return v;
    }
  }
  return access_inst;
}

SSAPtr LocalValueNumbering::FindValue(
    const std::shared_ptr<CallInst> &call_inst) {
  auto callee = call_inst->Callee();
  auto it     = FunctionInfos().find(callee.get());
  if (it == FunctionInfos().end() || !it->second.IsPure())
    return call_inst;

  for (auto [k, v] : _value_number) {
    //    auto [k, v] = _value_number[i];
    auto call_value = dyn_cast<CallInst>(k);
    if (call_value && (call_value->Callee() == call_inst->Callee())) {
      DBG_ASSERT(call_inst->param_size() == call_value->param_size(),
                 "parameters size of call instructions are different");
      if (call_inst->param_size() == 0)
        return v;
      for (auto idx = 0; idx < call_inst->param_size(); idx++) {
        if (ValueOf(call_inst->Param(idx)) != ValueOf(call_value->Param(idx))) {
          goto next_call_value;
        }
      }
      return v;
    }
  next_call_value:;
  }
  return call_inst;
}

SSAPtr LocalValueNumbering::FindValue(
    const std::shared_ptr<ICmpInst> &icmp_inst) {
  auto isrev = [](front::Operator a, front::Operator b) {
    return (a == front::Operator::SLess && b == front::Operator::SGreat) ||
           (a == front::Operator::SGreat && b == front::Operator::SLess) ||
           (a == front::Operator::SGreatEq && b == front::Operator::SLessEq) ||
           (a == front::Operator::SLessEq && b == front::Operator::SGreatEq);
  };

  auto op   = icmp_inst->op();
  auto lhs1 = ValueOf(icmp_inst->LHS());
  auto rhs1 = ValueOf(icmp_inst->RHS());

  for (auto [k, v] : _value_number) {
    //    auto [k, v] = _value_number[i];
    auto num_value = dyn_cast<ICmpInst>(k);
    if (num_value && (num_value != icmp_inst)) {
      front::Operator op2  = num_value->op();
      auto            lhs2 = ValueOf(num_value->LHS());
      auto            rhs2 = ValueOf(num_value->RHS());

      bool same = false;
      if (op == op2) {
        if (lhs1 == lhs2 && rhs1 == rhs2) {
          same = true;
        }
      } else if (lhs1 == rhs2 && rhs1 == lhs2 && isrev(op, op2)) {
        same = true;
      }
      if (same)
        return v;
    }
  }
  return icmp_inst;
}

SSAPtr LocalValueNumbering::ValueOf(const SSAPtr &value) {
  if (auto inst = dyn_cast<Instruction>(value)) {
    if (inst->getParent() != _cur_block) {
      return value;
    }
  }

  auto it = _value_number.find(value);
  if (it != _value_number.end())
    return it->second;
  if (auto const_value = dyn_cast<ConstantInt>(value)) {
    // need to handle const value
    it = std::find_if(
        _value_number.begin(), _value_number.end(),
        [value, &const_value](const std::pair<SSAPtr, SSAPtr> &kv) {
          if (kv.first == value)
            return true;

          auto const_kv = dyn_cast<ConstantInt>(kv.first);
          if (const_kv && (const_value->value() == const_kv->value())) {
            if (const_value->type()->IsIdentical(const_kv->type()))
              return true;
          }

          return false;
        });
    if (it != _value_number.end())
      return it->second;
  }

  auto [res, state] = _value_number.emplace(value, value);
  DBG_ASSERT(state == true, "insert new value failed");

  // find any way
  if (auto binary_inst = dyn_cast<BinaryOperator>(value)) {
    res->second = FindValue(binary_inst);
  } else if (auto access_inst = dyn_cast<AccessInst>(value)) {
    res->second = FindValue(access_inst);
  } else if (auto call_inst = dyn_cast<CallInst>(value)) {
    res->second = FindValue(call_inst);
  } else if (auto icmp_inst = dyn_cast<ICmpInst>(value)) {
    res->second = FindValue(icmp_inst);
  }

  return res->second;
}

void LocalValueNumbering::RunLocalValueNumbering(const FuncPtr &F) {
  constexpr std::size_t kLocalValueNumberingBlockLimit = 256;
  auto                  entry               = F->entry();
  auto                  rpo = _blkWalker.RPOTraverse(entry.get());

  for (const auto &BB : rpo) {
    // Keep value numbering local to a block until we have dominance-aware
    // leader selection again. Cross-block reuse is currently too fragile.
    _cur_block = BB;
    _value_number.clear();
    auto enable_value_numbering =
        BB->insts().size() <= kLocalValueNumberingBlockLimit;
    for (auto it = BB->insts().begin(); it != BB->inst_end();) {
      auto next = std::next(it);
      if (auto binary_inst = dyn_cast<BinaryOperator>(*it)) {
        if (enable_value_numbering)
          Replace(binary_inst, ValueOf(binary_inst));
      } else if (auto call_inst = dyn_cast<CallInst>(*it)) {
        auto callee = call_inst->Callee();
        auto pure   = FunctionInfos().find(callee.get());
        if (enable_value_numbering && pure != FunctionInfos().end() &&
            pure->second.IsPure()) {
          Replace(call_inst, ValueOf(call_inst));
        }
      } else if (auto access_inst = dyn_cast<AccessInst>(*it)) {
        if (enable_value_numbering)
          Replace(access_inst, ValueOf(access_inst));
      } else if (auto icmp_inst = dyn_cast<ICmpInst>(*it)) {
        if (enable_value_numbering)
          Replace(icmp_inst, ValueOf(icmp_inst));
      }
      it = next;
    }
  }
  _cur_block = nullptr;
}

void RegisterLocalValueNumberingPass() {
  RegisterPassCliMetadata({
      "LocalValueNumbering",
      "local-value-numbering",
      {"lvn"},
      "local value numbering and CSE",
  });
  static PassRegisterFactory<LocalValueNumberingFactory> registry;
}

} // namespace lava::opt
