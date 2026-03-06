#ifndef LAVA_SANITIZE_IR_H
#define LAVA_SANITIZE_IR_H

#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

class SanitizeIR : public ModulePass {
public:
  bool runOnModule(Module &M) final;
};

class SanitizeIRFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<SanitizeIR>();
    return std::make_shared<PassInfo>(pass, "SanitizeIR", false, 0,
                                      SANITIZE_IR);
  }
};

} // namespace lava::opt

#endif // LAVA_SANITIZE_IR_H
