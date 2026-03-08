#include "opt/analysis/expression_analysis.h"

#include <unordered_set>

#include "opt/register.h"

int ExpressionAnalysisPass;

namespace {

const std::vector<lava::opt::ExprOccurrence> &EmptyExprOccurrences() {
  static const std::vector<lava::opt::ExprOccurrence> empty;
  return empty;
}

} // namespace

namespace lava::opt {

void ExpressionAnalysisResult::Clear() {
  _expr_table.Clear();
  _occurrences.clear();
  _occurrences_by_block.clear();
  _occurrences_by_expr.clear();
  _occurrence_by_inst.clear();
  _expr_by_value.clear();
  _canonical_values.clear();
}

void ExpressionAnalysisResult::RecordCanonical(const SSAPtr &value,
                                               const SSAPtr &canonical) {
  if (value == nullptr)
    return;

  auto [it, inserted] = _canonical_values.emplace(value.get(), canonical);
  if (!inserted) {
    DBG_ASSERT(it->second == canonical, "canonical value mismatch");
  }
}

ExprId ExpressionAnalysisResult::RecordExpression(const ExprKey      &key,
                                                  const SSAPtr      &canonical,
                                                  const InstPtr     &instruction,
                                                  std::size_t        instruction_index) {
  DBG_ASSERT(instruction != nullptr, "expression occurrence is null");
  auto expr_id         = _expr_table.LookupOrInsertId(key, canonical);
  auto canonical_value = _expr_table.GetCanonical(expr_id);

  RecordCanonical(instruction, canonical_value);

  auto [expr_it, inserted] = _expr_by_value.emplace(instruction.get(), expr_id);
  if (!inserted) {
    DBG_ASSERT(expr_it->second == expr_id,
               "instruction recorded with different expression id");
    expr_it->second = expr_id;
  }

  ExprOccurrence occurrence{
      expr_id,
      instruction->getParent(),
      instruction,
      instruction_index,
  };
  _occurrences.push_back(occurrence);
  _occurrences_by_block[occurrence.block].push_back(occurrence);
  _occurrences_by_expr[expr_id].push_back(occurrence);

  auto [occ_it, occ_inserted] =
      _occurrence_by_inst.emplace(instruction.get(), occurrence);
  DBG_ASSERT(occ_inserted, "duplicate expression occurrence");
  static_cast<void>(occ_it);

  return expr_id;
}

std::optional<ExprId>
ExpressionAnalysisResult::LookupExprId(const ExprKey &key) const {
  return _expr_table.LookupId(key);
}

std::optional<ExprId>
ExpressionAnalysisResult::LookupExprId(const SSAPtr &value) const {
  if (value == nullptr)
    return std::nullopt;

  auto it = _expr_by_value.find(value.get());
  if (it == _expr_by_value.end())
    return std::nullopt;
  return it->second;
}

SSAPtr ExpressionAnalysisResult::LookupCanonicalValue(const ExprKey &key) const {
  auto expr_id = LookupExprId(key);
  if (!expr_id.has_value())
    return nullptr;
  return CanonicalValue(*expr_id);
}

const SSAPtr &ExpressionAnalysisResult::CanonicalValue(ExprId expr_id) const {
  return _expr_table.GetCanonical(expr_id);
}

SSAPtr ExpressionAnalysisResult::Canonicalize(const SSAPtr &value) const {
  if (value == nullptr)
    return nullptr;

  auto it = _canonical_values.find(value.get());
  return it == _canonical_values.end() ? value : it->second;
}

const ExprKey &ExpressionAnalysisResult::Key(ExprId expr_id) const {
  return _expr_table.GetKey(expr_id);
}

const std::vector<ExprOccurrence> &
ExpressionAnalysisResult::Occurrences(BasicBlock *block) const {
  auto it = _occurrences_by_block.find(block);
  return it == _occurrences_by_block.end() ? EmptyExprOccurrences()
                                           : it->second;
}

const std::vector<ExprOccurrence> &
ExpressionAnalysisResult::Occurrences(ExprId expr_id) const {
  auto it = _occurrences_by_expr.find(expr_id);
  return it == _occurrences_by_expr.end() ? EmptyExprOccurrences()
                                          : it->second;
}

const ExprOccurrence *
ExpressionAnalysisResult::Occurrence(const Instruction *instruction) const {
  auto it = _occurrence_by_inst.find(instruction);
  return it == _occurrence_by_inst.end() ? nullptr : &it->second;
}

void ExpressionAnalysis::initialize() {
  _cur_func = nullptr;
  _expr_context.Reset();
  _expr_context.SetFunctionInfos(
      PassManager::RequireAnalysisResult<FuncInfoMap>("FunctionInfoPass"));
}

void ExpressionAnalysis::finalize() {
  _cur_func = nullptr;
  _expr_context.Reset();
}

void ExpressionAnalysis::AnalyzeBlock(ExpressionAnalysisResult &result,
                                      BasicBlock               *block) {
  std::size_t instruction_index = 0;
  for (const auto &inst : block->insts()) {
    auto canonical_inst = _expr_context.Canonicalize(inst);
    result.RecordCanonical(inst, canonical_inst);

    for (unsigned i = 0; i < inst->size(); ++i) {
      const auto &operand = (*inst)[i].value();
      if (operand == nullptr)
        continue;
      result.RecordCanonical(operand, _expr_context.Canonicalize(operand));
    }

    if (auto key = _expr_context.BuildKey(inst)) {
      result.RecordExpression(*key, canonical_inst, inst, instruction_index);
    }

    ++instruction_index;
  }
}

bool ExpressionAnalysis::runOnFunction(const FuncPtr &F) {
  if (F->is_decl())
    return false;

  _cur_func = F.get();
  _expr_context.Reset();
  _expr_context.SetFunctionInfos(FunctionInfos());

  auto &result = GetMutableExpressionAnalysisMap()[_cur_func];
  result.Clear();

  for (const auto &arg : F->args()) {
    if (arg != nullptr)
      result.RecordCanonical(arg, _expr_context.Canonicalize(arg));
  }

  std::unordered_set<BasicBlock *> visited;
  auto                              entry = F->entry_block();
  if (entry != nullptr) {
    for (auto *block : _blk_walker.RPOTraverse(entry.get())) {
      visited.insert(block);
      AnalyzeBlock(result, block);
    }
  }

  for (const auto &block : *F) {
    if (!visited.insert(block.get()).second)
      continue;
    AnalyzeBlock(result, block.get());
  }

  return false;
}

void RegisterExpressionAnalysisPass() {
  RegisterPassCliMetadata({
      "ExpressionAnalysis",
      "expression-analysis",
      {},
      "compute per-function scalar expression ids and occurrences",
  });
  static PassRegisterFactory<ExpressionAnalysisFactory> registry;
}

} // namespace lava::opt
