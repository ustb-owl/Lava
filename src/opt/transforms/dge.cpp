#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "mid/ir/castssa.h"
#include "opt/pass_manager.h"

int DeadGlobalCodeElimination;

namespace lava::opt {

/*
 * Dead global code elimination
 * 1. delete unused function
 * 2. delete unused internal/inline functions and global variables
 */
class DeadGlobalCodeElimination : public ModulePass {
public:
  bool runOnModule(Module &M) final {
    bool changed = false;

    // handle global variables
    auto &glb_vars = M.GlobalVars();
    for (auto it = glb_vars.begin(); it != glb_vars.end();) {
      auto glbVal = *it;
      if (glbVal->uses().empty()) {
        glbVal->logger()->LogWarning("unused global variable");
        it = glb_vars.erase(it);
        changed |= !changed;
      } else {
        it++;
      }
    }

    // handle functions
    auto &funcs = M.Functions();
    for (auto it = funcs.begin(); it != funcs.end();) {
      (*it)->logger()->LogWarning("unused function");

      if ((*it)->uses().empty()) {

        // do not remove main function
        if ((*it)->GetFunctionName() == "main") {
          it++;
          continue;
        }

        for (const auto &block : *it->get()) {
          auto BB = dyn_cast<BasicBlock>(block.value());
          BB->DeleteSelf();
        }

        // clear all use of basic block
        (*it)->Clear();

        // remove from function list
        funcs.erase(it);

        changed |= !changed;
      } else {
        it++;
      }
    }

    return changed;
  }
};

class DeadGlobalCodeEliminationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DeadGlobalCodeElimination>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DeadGlobalCodeElimination", false, false, DEAD_GLOBAL_CODE_ELIMINATION);
    return passinfo;
  }
};

static PassRegisterFactory<DeadGlobalCodeEliminationFactory> registry;

}