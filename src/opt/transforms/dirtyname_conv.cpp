#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

int DirtyFunctionConvert;

namespace lava::opt {
/*
 * TODO: DIRTY_HACK
 * Convert starttime to _sysy_starttime, stoptime to _sysy_stoptime
 */

class DirtyFunctionNameConvert : public FunctionPass {
private:
  bool _changed = false;

  SSAPtr _start_time = nullptr;

public:
  bool runOnFunction(const FuncPtr &F) final {
    if (F->GetFunctionName() == "starttime") {
      F->SetName("_sysy_starttime");
      FixTypeAndParam(F);

    } else if (F->GetFunctionName() == "stoptime") {
      F->SetName("_sysy_stoptime");
      FixTypeAndParam(F);
    } else {
      // find all call instruction
      for (const auto &BB : *F) {
        auto block = dyn_cast<BasicBlock>(BB.value());
        for (auto it = block->inst_begin(); it !=block->insts().end(); it++) {
          if (auto call_inst = dyn_cast<CallInst>(*it)) {
            auto callee = dyn_cast<Function>(call_inst->Callee());
            DBG_ASSERT(callee != nullptr, "callee is not function");
            auto name = callee->GetFunctionName();
            if (name == "_sysy_starttime" || name == "_sysy_stoptime") {
              if (call_inst->size() == 2) continue;
              auto zero = std::make_shared<ConstantInt>(0);
              zero->set_type(MakePrimType(Type::Int32, true));
              call_inst->AddParam(zero);
            }

#if 0
            if (name == "_sysy_starttime") {
              _start_time = call_inst;
              it = block->insts().erase(it);
            } else if (name == "_sysy_stoptime") {
              it = block->insts().insert(it, _start_time);
            }
#endif
          }
        }
      }
      return _changed;
    }
    return true;
  }

  // _sysy_starttime(int lineno);
  void FixTypeAndParam(const FuncPtr &F) {
    // 1. set function type
    define::TypePtrList params;
    auto param_type = MakePrimType(Type::Int32, false);
    params.push_back(std::move(param_type));
    auto ret_type = MakePrimType(Type::Void, false);
    F->set_type(std::make_shared<define::FuncType>(std::move(params), ret_type, false));

    // 2. add argument
    auto arg_type = MakePrimType(Type::Int32, false);
    auto arg = std::make_shared<ArgRefSSA>(F, 0, "lineno");
    arg->set_type(arg_type);
    F->set_arg(0, arg);
  }
};

class DirtyFunctionNameConvertFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DirtyFunctionNameConvert>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DirtyFunctionNameConvert", false, 0, DIRTY_FUNCTION_CONV);
    return passinfo;
  }
};

static PassRegisterFactory<DirtyFunctionNameConvertFactory> registry;
}