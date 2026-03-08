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
    _changed = false;
    const auto &expressions =
        PassManager::RequireAnalysisResultOnFunction<ExpressionAnalysisMap>(
            "ExpressionAnalysis", F)
            .at(F.get());
    RunLocalValueNumbering(F, expressions);

    // Run cleanup passes after local value numbering/CSE.
    _changed |= PassManager::RunPassOnFunction("DeadCodeElimination", F);

    _changed |= PassManager::RunPassOnFunction("BlockSimplification", F);
    changed = _changed;
    changed_any |= changed;
  } while (changed);

  return changed_any;
}

void LocalValueNumbering::Replace(const InstPtr &inst, const SSAPtr &value) {
  if (inst != value) {
    inst->ReplaceBy(value);
    _changed = true;
  }
}

void LocalValueNumbering::RunLocalValueNumbering(
    const FuncPtr &F, const ExpressionAnalysisResult &expressions) {
  constexpr std::size_t kLocalValueNumberingBlockLimit = 256;

  for (const auto &block : *F) {
    if (block->insts().size() > kLocalValueNumberingBlockLimit)
      continue;

    std::unordered_map<ExprId, SSAPtr> leaders;
    for (const auto &occurrence : expressions.Occurrences(block.get())) {
      auto [it, inserted] =
          leaders.emplace(occurrence.expr_id, occurrence.instruction);
      if (!inserted)
        Replace(occurrence.instruction, it->second);
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
