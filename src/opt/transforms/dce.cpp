#include "dce.h"

int DeadCodeElimination;

namespace lava::opt {


bool DeadCodeElimination::runOnFunction(const FuncPtr &F) {
  _changed = false;
  if (F->is_decl()) return _changed;

  Mark(F);
  Sweep(F);
  return _changed;
}

bool DeadCodeElimination::IsCriticalInstruction(const SSAPtr &ptr) {
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

void DeadCodeElimination::Mark(const FuncPtr &F) {
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

void DeadCodeElimination::Sweep(const FuncPtr &F) {
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

          // clear operand
          for (auto &use : *inst) {
            use.set(nullptr);
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


static PassRegisterFactory<DeadCodeEliminationFactory> registry;


}