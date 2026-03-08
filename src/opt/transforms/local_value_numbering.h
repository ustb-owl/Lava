#ifndef LAVA_LOCAL_VALUE_NUMBERING_H
#define LAVA_LOCAL_VALUE_NUMBERING_H

#include <algorithm>

#include "common/casting.h"
#include "opt/analysis/funcanalysis.h"
#include "opt/blkwalker.h"
#include "opt/expression_context.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {
class LocalValueNumbering : public FunctionPass {
private:
  bool              _changed;
  BlockWalker       _blkWalker;
  ExpressionContext _expr_context;

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final;

  void finalize() final;

  void Replace(const InstPtr &inst, const SSAPtr &value);

  void RunLocalValueNumbering(const FuncPtr &F);
};

class LocalValueNumberingFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass     = std::make_shared<LocalValueNumbering>();
    auto passinfo = std::make_shared<PassInfo>(pass, "LocalValueNumbering",
                                               false, 1, LOCAL_VALUE_NUMBERING);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};
} // namespace lava::opt

#endif // LAVA_LOCAL_VALUE_NUMBERING_H
