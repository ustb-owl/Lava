#include <algorithm>

#include "common/casting.h"
#include "lib/debug.h"
#include "opt/analysis/funcanalysis.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"
#include "opt/register.h"

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
    bool        changed = false;
    const auto &func_infos =
        PassManager::RequireAnalysisResult<FuncInfoMap>("FunctionInfoPass");

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
      // do not remove main function
      if ((*it)->GetFunctionName() == "main") {
        it++;
        continue;
      }

      if (!func_infos.contains(it->get())) {
        (*it)->logger()->LogWarning("unused function");
        for (const auto &BB : *it->get()) {
          BB->DeleteSelf();
        }

        (*it)->ClearBlocks();

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
        std::make_shared<PassInfo>(pass, "DeadGlobalCodeElimination", false, 0,
                                   DEAD_GLOBAL_CODE_ELIMINATION);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};

void RegisterDeadGlobalCodeEliminationPass() {
  RegisterPassCliMetadata({
      "DeadGlobalCodeElimination",
      "dead-global-code-elimination",
      {"dge"},
      "remove unused functions and globals",
  });
  static PassRegisterFactory<DeadGlobalCodeEliminationFactory> registry;
}

} // namespace lava::opt
