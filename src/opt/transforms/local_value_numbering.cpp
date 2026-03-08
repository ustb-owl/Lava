#include "local_value_numbering.h"

#include "opt/register.h"

int LocalValueNumberingPass;

namespace lava::opt {

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
    _expr_context.Reset();

    // Run cleanup passes after local value numbering/CSE.
    _changed |= PassManager::RunPassOnFunction("DeadCodeElimination", F);

    _changed |= PassManager::RunPassOnFunction("BlockSimplification", F);
    changed = _changed;
    changed_any |= changed;
  } while (changed);

  return changed_any;
}

void LocalValueNumbering::initialize() {
  const auto &function_infos =
      PassManager::RequireAnalysisResult<FuncInfoMap>("FunctionInfoPass");
  _expr_context.Reset();
  _expr_context.SetFunctionInfos(function_infos);
}

void LocalValueNumbering::finalize() { _expr_context.Reset(); }

void LocalValueNumbering::Replace(const InstPtr &inst, const SSAPtr &value) {
  if (inst != value) {
    inst->ReplaceBy(value);
    _changed = true;
    _expr_context.Forget(inst);
  }
}

void LocalValueNumbering::RunLocalValueNumbering(const FuncPtr &F) {
  constexpr std::size_t kLocalValueNumberingBlockLimit = 256;
  auto                  entry                          = F->entry();
  auto                  rpo = _blkWalker.RPOTraverse(entry.get());

  for (const auto &BB : rpo) {
    // Keep value numbering local to a block until we have dominance-aware
    // leader selection again. Cross-block reuse is currently too fragile.
    _expr_context.ResetForBlock(BB);
    auto enable_value_numbering =
        BB->insts().size() <= kLocalValueNumberingBlockLimit;
    for (auto it = BB->insts().begin(); it != BB->inst_end();) {
      auto next = std::next(it);
      if (enable_value_numbering && _expr_context.IsEligibleValue(*it))
        Replace(*it, _expr_context.Canonicalize(*it));
      it = next;
    }
  }
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
