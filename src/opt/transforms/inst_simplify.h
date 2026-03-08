#ifndef LAVA_INST_SIMPLIFY_H
#define LAVA_INST_SIMPLIFY_H

#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

class InstSimplify : public FunctionPass {
private:
  bool _changed = false;

  bool SimplifyBinary(const std::shared_ptr<BinaryOperator> &binary_inst);

  bool SimplifyCmp(const std::shared_ptr<ICmpInst> &icmp_inst);

  bool SimplifyPhi(const std::shared_ptr<PhiNode> &phi_node);

  bool ReplaceAndEraseIfChanged(const InstPtr &inst, const SSAPtr &value);

public:
  bool runOnFunction(const FuncPtr &F) final;

  void initialize() final { _changed = false; }

  void finalize() final {}
};

class InstSimplifyFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<InstSimplify>();
    return std::make_shared<PassInfo>(pass, "InstSimplify", false, 1,
                                      INST_SIMPLIFY);
  }
};

} // namespace lava::opt

#endif // LAVA_INST_SIMPLIFY_H
