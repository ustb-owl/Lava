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

    _changed = false;
    RunLocalValueNumbering(F);

    // Clear the leader cache before cleanup passes.
    _leader_cache.clear();

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
  _expr_table.Clear();
  _constant_ints.clear();
  static_cast<void>(
      PassManager::RequireAnalysisResult<FuncInfoMap>("FunctionInfoPass"));
}

void LocalValueNumbering::finalize() {
  _leader_cache.clear();
  _expr_table.Clear();
  _constant_ints.clear();
  _cur_block = nullptr;
}

void LocalValueNumbering::Replace(const InstPtr &inst, const SSAPtr &value) {
  if (inst != value) {
    inst->ReplaceBy(value);
    _changed = true;

    auto res = std::find_if(_leader_cache.begin(), _leader_cache.end(),
                            [inst](const std::pair<SSAPtr, SSAPtr> &kv) {
                              return kv.first == inst;
                            });
    if (res != _leader_cache.end()) {
      _leader_cache.erase(res);
    }
  }
}

SSAPtr LocalValueNumbering::CanonicalizeConstant(
    const std::shared_ptr<ConstantInt> &constant) {
  ConstantIntKey key{CanonicalTypeId(constant->type()), constant->value()};
  auto [it, _] = _constant_ints.emplace(key, constant);
  return it->second;
}

SSAPtr LocalValueNumbering::ValueOf(const SSAPtr &value) {
  if (auto inst = dyn_cast<Instruction>(value)) {
    if (inst->getParent() != _cur_block) {
      return value;
    }
  }

  auto it = _leader_cache.find(value);
  if (it != _leader_cache.end())
    return it->second;

  SSAPtr leader = value;
  if (auto const_value = dyn_cast<ConstantInt>(value)) {
    leader = CanonicalizeConstant(const_value);
  } else if (auto key = BuildExprKey(
                 value,
                 [this](const SSAPtr &operand) { return ValueOf(operand); },
                 [this](const std::shared_ptr<CallInst> &call) {
                   return IsPureCall(call);
                 })) {
    leader = _expr_table.LookupOrInsert(*key, value);
  }

  auto [res, state] = _leader_cache.emplace(value, leader);
  DBG_ASSERT(state == true, "insert new value failed");
  return res->second;
}

void LocalValueNumbering::RunLocalValueNumbering(const FuncPtr &F) {
  constexpr std::size_t kLocalValueNumberingBlockLimit = 256;
  auto                  entry                          = F->entry();
  auto                  rpo = _blkWalker.RPOTraverse(entry.get());

  for (const auto &BB : rpo) {
    // Keep value numbering local to a block until we have dominance-aware
    // leader selection again. Cross-block reuse is currently too fragile.
    _cur_block = BB;
    _leader_cache.clear();
    _expr_table.Clear();
    _constant_ints.clear();
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
      } else if (auto cast_inst = dyn_cast<CastInst>(*it)) {
        if (enable_value_numbering)
          Replace(cast_inst, ValueOf(cast_inst));
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
