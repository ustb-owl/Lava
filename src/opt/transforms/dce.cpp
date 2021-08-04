#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

#include "opt/analysis/postdominance.h"

int DeadCodeElimination;

namespace lava::opt {

/*
 aggressive dead code elimination

 use mark and sweep algorithm
 */
class DeadCodeElimination : public FunctionPass {
private:
  bool _changed;
  std::vector<User *> _work_list;
  std::unordered_set<Value *> _critical_list;

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    Mark(F);
    Sweep(F);
    return _changed;
  }

  void initialize() final {}

  void finalize() final {
    DBG_ASSERT(_work_list.empty(), "work list is not empty");
    _critical_list.clear();
  }

private:
  /*
   An operation is critical if:
   1. It sets return values for the function
   2. It is an input/output statement
   3. It affects the value which may be accessible from outside the current function
   */
  bool IsCriticalInstruction(const SSAPtr &ptr) {
    if (auto inst = dyn_cast<Instruction>(ptr)) {
      // TODO: check if callee has side effect
      if (IsSSA<ReturnInst>(inst) || IsSSA<CallInst>(inst) ||
          IsSSA<BranchInst>(inst) || IsSSA<JumpInst>(inst)) {
        return true;
      } else if (IsSSA<StoreInst>(inst)) {
        // TODO: check if is global
        // TODO: how to check array
        return true;
      }
    }
    return false;
  }

  /* Mark critical instructions */
  void Mark(const FuncPtr &F) {
    _work_list.clear();
    _critical_list.clear();


    // mark critical instruction
    for (const auto &it : *F) {
      auto BB = dyn_cast<BasicBlock>(it.value());
      for (const auto &inst : BB->insts()) {
        if (IsCriticalInstruction(inst)) {
          _critical_list.insert(inst.get());
          _work_list.push_back(dyn_cast<User>(inst).get());
        }
      }
    }

    // mark operands of critical instructions
    while (!_work_list.empty()) {
      auto inst = _work_list.back();
      _work_list.pop_back();

      // insert operands into critical set
      DBG_ASSERT(inst->isInstruction(), "get instruction failed");
      for (const auto &op_use : *inst) {
        auto val = op_use.value();
        if (!val || _critical_list.count(val.get())) continue;
        if (val->isInstruction()) {
          _critical_list.insert(val.get());
          _work_list.push_back(dyn_cast<User>(val).get());
        }
      }

    }
  }

  void Sweep(const FuncPtr &F) {
    for (const auto &bb_use : *F) {
      auto block = dyn_cast<BasicBlock>(bb_use.value());
      for (auto it = block->inst_begin(); it != block->insts().end();) {

        // handle unmarked instruction
        auto inst = dyn_cast<Instruction>(*it);
        DBG_ASSERT(inst != nullptr, "cast to instruction failed");
        if (_critical_list.find(inst.get()) == _critical_list.end()) {
          auto class_id = inst->classId();
          if ((class_id != ClassId::BranchInstId) && (class_id != ClassId::JumpInstId)) {
            // perform check
//            for (const auto &use : *inst) {
//              DBG_ASSERT(!_critical_list.count(use.value().get()), "operand of this instruction is marked as used");
//              static_cast<void>(use);
//            }
            for (const auto &use : inst->uses()) {
              DBG_ASSERT(!_critical_list.count(use->getUser()), "user of this instruction is marked as used");
              static_cast<void>(use);
            }
            // break circular reference
            (*it)->ReplaceBy(nullptr);
            it = block->insts().erase(it);
            _changed = true;
          } else {
            it++;
          }
        } else {
          it++;
        }
      }
    }
  }
};

class DeadCodeEliminationFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DeadCodeElimination>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DeadCodeElimination", false, false, DEAD_CODE_ELIMINATION);

    return passinfo;
  }
};

//static PassRegisterFactory<DeadCodeEliminationFactory> registry;

}