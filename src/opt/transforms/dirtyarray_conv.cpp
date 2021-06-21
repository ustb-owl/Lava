#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "mid/ir/castssa.h"
#include "opt/pass_manager.h"

int DirtyArrayConvert;

namespace lava::opt {

/*
 * Convert type of array which in function parameters into i32*
 *
 * eg:
 * [3 * i32]  -> i32**
 * [4 * i32]* -> i32**
 * ....
 */
class DirtyArrayConvert : public FunctionPass {
private:
  bool _changed = false;

public:
  bool runOnFunction(const FuncPtr &F) final {
    SSAPtr entry;
    if (F->empty()) return _changed;

    for (auto &it : *F) {
      if (entry == nullptr) entry = it.get();
      break;
    }

    auto entryBlock = dyn_cast<BasicBlock>(entry);
    for (auto &it : entryBlock->insts()) {
      auto inst = dyn_cast<Instruction>(it);
      switch (inst->opcode()) {
        case Instruction::Alloca: {

          auto alloca = dyn_cast<AllocaInst>(it);
          auto pointeeType = alloca->type()->GetDerefedType();
          if (pointeeType->IsArray() || pointeeType->IsPointer()) {

            // reset its user's operands type
            for (auto &use : alloca->uses()) {
              auto user = use->getUser();
              if (user->isInstruction()) {
                auto userInst = static_cast<Instruction *>(user);
                switch (userInst->opcode()) {
                  case Instruction::Store: {
                    auto store = static_cast<StoreInst *>(userInst);

                    // check if alloca is the dst
                    if (store->pointer() == alloca) {
                      if (store->value()->IsArgument()) {
                        auto new_type = MakePointer(MakePrimType(Type::Int32, pointeeType->IsRightValue()));
                        new_type = MakePointer(new_type);
                        alloca->set_type(new_type);
                        store->value()->set_type(alloca->type()->GetDerefedType());
                      } else {
                        goto out;
                      }
                    }
                    break;
                  }

                  case Instruction::Load: {
                    auto load = static_cast<LoadInst *>(userInst);

                    if (load->Pointer() == alloca) {
                      load->set_type(alloca->type()->GetDerefedType());
                    }
                    break;
                  }

                }
              }
            }
          }
        }


      }
      out:
      continue;
    }

    return _changed;
  }

};

class DirtyArrayConvertFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<DirtyArrayConvert>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "DirtyArrayConvert", false, false, DIRTY_ARRAY_CONV);
    return passinfo;
  }
};

static PassRegisterFactory<DirtyArrayConvertFactory> registry;

}