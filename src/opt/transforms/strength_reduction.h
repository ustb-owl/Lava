#ifndef LAVA_STRENGTH_REDUCTION_H
#define LAVA_STRENGTH_REDUCTION_H

#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

class StrengthReduction : public FunctionPass {
private:
  bool _changed = false;

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final { _changed = false; }

  void finalize() final {}
};

class StrengthReductionFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<StrengthReduction>();
    return std::make_shared<PassInfo>(pass, "StrengthReduction", false, 1,
                                      STRENGTH_REDUCTION);
  }
};

} // namespace lava::opt

#endif // LAVA_STRENGTH_REDUCTION_H
