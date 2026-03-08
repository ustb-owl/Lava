#ifndef LAVA_LOCAL_VALUE_NUMBERING_H
#define LAVA_LOCAL_VALUE_NUMBERING_H

#include <unordered_map>

#include "opt/analysis/expression_analysis.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {
class LocalValueNumbering : public FunctionPass {
private:
  bool _changed = false;

public:
  bool runOnFunction(const FuncPtr &F) final;

  void Replace(const InstPtr &inst, const SSAPtr &value);

  void RunLocalValueNumbering(const FuncPtr                  &F,
                              const ExpressionAnalysisResult &expressions);
};

class LocalValueNumberingFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass     = std::make_shared<LocalValueNumbering>();
    auto passinfo = std::make_shared<PassInfo>(pass, "LocalValueNumbering",
                                               false, 1, LOCAL_VALUE_NUMBERING);
    return passinfo;
  }
};
} // namespace lava::opt

#endif // LAVA_LOCAL_VALUE_NUMBERING_H
