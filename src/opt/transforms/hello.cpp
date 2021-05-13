#include <iostream>

#include "opt/pass.h"
#include "lib/debug.h"
#include "opt/pass_manager.h"

int HelloXY;

namespace lava::opt {
class HelloXYPass : public ModulePass {
public:

  bool runOnModule(Module &M) final {
    for (const auto &func : M.Functions()) {
      std::cout << "Hi! "
                << std::static_pointer_cast<Function>(func)->GetFunctionName()
                << std::endl;
    }
    return false;
  }

};

class HelloPassFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<HelloXYPass>();
    return std::make_shared<PassInfo>(pass, "HelloXYPass", false, 0);
  }
};

//static PassRegisterFactory<HelloPassFactory> registry;

}