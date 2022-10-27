#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

#include <iostream>

int TailRecursion;

namespace lava::opt {

class TailRecursion : public FunctionPass {
private:
  bool _changed{};

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl() || F->args().size() > 4) return _changed;
    CheckIfTailRecursion(F);
    std::cout << F->GetFunctionName() << ": " << F->is_tail_recursion() << std::endl;
    return _changed;
  }

  void initialize() final {}

  void finalize() final {}

  void CheckIfTailRecursion(const FuncPtr &F) {
    for (const auto &use : *F) {
      auto block = dyn_cast<BasicBlock>(use.value());
      for (auto it = block->insts().begin(); it != block->insts().end(); it++) {
        if (auto ret_inst = dyn_cast<ReturnInst>(*it)) {
          auto ret_value = ret_inst->RetVal();

          // if (...) return F()
          // else return x;
          // phi1 = phi [F()], [x]
          if (auto phi_node = dyn_cast<PhiNode>(ret_value)) {
            for (std::size_t i = 0; i < phi_node->size(); i++) {
              if (auto call_inst = dyn_cast<CallInst>((*phi_node)[i].value())) {
                if (dyn_cast<Function>(call_inst->Callee()) == F) {

                  // check if the call instruction is the last inst of its block
                  auto pred = dyn_cast<BasicBlock>((*(phi_node->parent_block()))[i].value());
                  DBG_ASSERT(pred != nullptr, "get pred block of phi-node failed");
                  auto inst = *std::prev(std::prev(pred->insts().end()));
                  if (inst == call_inst) {
                    F->SetIsRecursion(true);
                    call_inst->SetIsTailCall(true);
                  }
                }
              }
            }
          } else if (auto call_inst = dyn_cast<CallInst>(ret_value)) {
            // return F();
            // %1 = call F; ret %1
            if (dyn_cast<Function>(call_inst->Callee()) == F) {
              for (const auto &bb_use : *F) {
                auto bb = dyn_cast<BasicBlock>(bb_use.value());
                for (auto inst_it = bb->insts().begin(); inst_it != bb->insts().end(); inst_it++) {
                  if (*inst_it == call_inst) {
                    if (std::next(std::next(inst_it)) == bb->insts().end()) {
                      goto found;
                    }
                  }
                }
              }
              found:
              F->SetIsRecursion(true);
              call_inst->SetIsTailCall(true);
            }
          }
        }
      }
    }
  }
};

class TailRecursionFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<TailRecursion>();
    auto passinfo = std::make_shared<PassInfo>(pass, "TailRecursion", false, 2, TAIL_RECURSION);

    return passinfo;
  }
};

static PassRegisterFactory<TailRecursionFactory> registry;

}

