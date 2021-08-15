#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int LocalMemoryProp;

namespace lava::opt {

class LocalMemoryPropagation : public FunctionPass {
private:
  bool _changed;

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;

    TRACE0();

    return false;
  }

  void initialize() final {}

  void finalize() final {}

};

class LocalMemoryPropagationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<LocalMemoryPropagation>();
    auto passinfo = std::make_shared<PassInfo>(pass, "LocalMemoryPropagation", false, 2, LOCAL_MEM_PROP);

    return passinfo;
  }
};

static PassRegisterFactory<LocalMemoryPropagationFactory> registry;

}