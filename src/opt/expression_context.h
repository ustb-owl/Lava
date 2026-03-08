#ifndef LAVA_EXPRESSION_CONTEXT_H
#define LAVA_EXPRESSION_CONTEXT_H

#include <unordered_map>

#include "opt/analysis/funcanalysis.h"
#include "opt/expression_key.h"

namespace lava::opt {

bool IsPureScalarCall(const std::shared_ptr<mid::CallInst> &call_inst,
                      const FuncInfoMap                    &function_infos);

class ExpressionContext {
private:
  const FuncInfoMap                           *_function_infos = nullptr;
  mid::BasicBlock                             *_current_block  = nullptr;
  std::unordered_map<mid::SSAPtr, mid::SSAPtr> _leader_cache;
  ExprTable                                    _expr_table;
  ConstantIntTable                             _constant_ints;

  mid::SSAPtr
  CanonicalizeConstant(const std::shared_ptr<mid::ConstantInt> &constant);

public:
  void SetFunctionInfos(const FuncInfoMap &function_infos);

  void Reset();

  void ResetForBlock(mid::BasicBlock *block);

  void Forget(const mid::SSAPtr &value);

  bool IsPureScalarCall(const mid::SSAPtr &value) const;

  bool IsEligibleValue(const mid::SSAPtr &value) const;

  mid::SSAPtr Canonicalize(const mid::SSAPtr &value);
};

} // namespace lava::opt

#endif // LAVA_EXPRESSION_CONTEXT_H
