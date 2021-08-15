#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int GlobalConstPropagation;

namespace lava::opt {
/*
 * replace load global constant with mov
 */
class GlobalConstPropagation : public ModulePass {
private:
  bool _changed;
  std::unordered_set<SSAPtr> _global_vars;
  std::unordered_map<SSAPtr, SSAPtr> _alias;

public:
  bool runOnModule(Module &M) final {
    _changed = false;

    for (const auto &glob : M.GlobalVars()) {
      if (glob->type()->IsPointer() && glob->type()->GetDerefedType()->IsInteger()) {
        auto glb_var = dyn_cast<GlobalVariable>(glob);
        if (glb_var->init() != nullptr) {
          _global_vars.insert(glob);
        }
      }
    }

    RemoveChangedVar(M);
    ReplaceAlias();
    return _changed;
  }

  void finalize() final {
    _global_vars.clear();
    _alias.clear();
  }

  void RemoveChangedVar(Module &M) {
    for (const auto &F : M.Functions()) {
      for (const auto &BB : *F) {
        auto block = dyn_cast<BasicBlock>(BB.value());
        for (const auto &inst : block->insts()) {
          if (auto load_inst = dyn_cast<LoadInst>(inst)) {
            auto it = _global_vars.find(load_inst->Pointer());
            if (it == _global_vars.end()) continue;;
            _alias.insert({load_inst, *it});
          } else if (auto store_inst = dyn_cast<StoreInst>(inst)) {
            auto it = _global_vars.find(store_inst->pointer());
            if (it == _global_vars.end()) continue;
            _global_vars.erase(it);
          }
        }
      }
    }

    for (auto it = _alias.begin(); it != _alias.end();) {
      auto res = _global_vars.find(it->second);
      if (res == _global_vars.end()) {
        it = _alias.erase(it);
      } else {
        it++;
      }
    }
  }

  void ReplaceAlias() {
    for (auto &[k, v] : _alias) {
      auto global_var = dyn_cast<GlobalVariable>(v);
      DBG_ASSERT(global_var->init() != nullptr, "global variable hasn't init value");
      auto init = dyn_cast<ConstantInt>(global_var->init());
      k->ReplaceBy(init);
    }
  }
};


class GlobalConstPropagationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<GlobalConstPropagation>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "GlobalConstPropagation", false, 0, GLOBAL_CONST_PROP);
    return passinfo;
  }
};

static PassRegisterFactory<GlobalConstPropagationFactory> registry;

}
