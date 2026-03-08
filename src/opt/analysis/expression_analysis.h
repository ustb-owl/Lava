#ifndef LAVA_EXPRESSION_ANALYSIS_H
#define LAVA_EXPRESSION_ANALYSIS_H

#include <optional>
#include <unordered_map>
#include <vector>

#include "opt/analysis/funcanalysis.h"
#include "opt/blkwalker.h"
#include "opt/expression_context.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

struct ExprOccurrence {
  ExprId           expr_id           = kInvalidExprId;
  mid::BasicBlock *block             = nullptr;
  mid::InstPtr     instruction       = nullptr;
  std::size_t      instruction_index = 0;
};

class ExpressionAnalysisResult {
private:
  ExprTable _expr_table;
  std::vector<ExprOccurrence> _occurrences;
  std::unordered_map<mid::BasicBlock *, std::vector<ExprOccurrence>>
      _occurrences_by_block;
  std::unordered_map<ExprId, std::vector<ExprOccurrence>>
      _occurrences_by_expr;
  std::unordered_map<const mid::Instruction *, ExprOccurrence>
      _occurrence_by_inst;
  std::unordered_map<const mid::Value *, ExprId>      _expr_by_value;
  std::unordered_map<const mid::Value *, mid::SSAPtr> _canonical_values;

public:
  void Clear();

  void RecordCanonical(const mid::SSAPtr &value, const mid::SSAPtr &canonical);

  ExprId RecordExpression(const ExprKey       &key,
                          const mid::SSAPtr   &canonical,
                          const mid::InstPtr  &instruction,
                          std::size_t          instruction_index);

  std::optional<ExprId> LookupExprId(const ExprKey &key) const;

  std::optional<ExprId> LookupExprId(const mid::SSAPtr &value) const;

  mid::SSAPtr LookupCanonicalValue(const ExprKey &key) const;

  const mid::SSAPtr &CanonicalValue(ExprId expr_id) const;

  mid::SSAPtr Canonicalize(const mid::SSAPtr &value) const;

  const ExprKey &Key(ExprId expr_id) const;

  const ExprTable &Expressions() const { return _expr_table; }

  const std::vector<ExprOccurrence> &Occurrences() const { return _occurrences; }

  const std::vector<ExprOccurrence> &Occurrences(mid::BasicBlock *block) const;

  const std::vector<ExprOccurrence> &Occurrences(ExprId expr_id) const;

  const ExprOccurrence *Occurrence(const mid::Instruction *instruction) const;
};

using ExpressionAnalysisMap =
    std::unordered_map<mid::Function *, ExpressionAnalysisResult>;

class ExpressionAnalysis : public FunctionPass {
private:
  Function          *_cur_func = nullptr;
  BlockWalker        _blk_walker;
  ExpressionContext  _expr_context;

  ExpressionAnalysisMap &GetMutableExpressionAnalysisMap() {
    return PassManager::GetMutableAnalysisResult<ExpressionAnalysisMap>(name());
  }

  const FuncInfoMap &FunctionInfos() const {
    return PassManager::GetAnalysisResult<FuncInfoMap>("FunctionInfoPass");
  }

  void AnalyzeBlock(ExpressionAnalysisResult &result, mid::BasicBlock *block);

public:
  void initialize() final;

  void finalize() final;

  bool runOnFunction(const FuncPtr &F) final;
};

class ExpressionAnalysisFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass     = std::make_shared<ExpressionAnalysis>();
    auto passinfo = std::make_shared<PassInfo>(pass, "ExpressionAnalysis",
                                               true, 0, EXPRESSION_ANALYSIS);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};

} // namespace lava::opt

#endif // LAVA_EXPRESSION_ANALYSIS_H
