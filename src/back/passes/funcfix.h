#ifndef LAVA_FUNCFIX_H
#define LAVA_FUNCFIX_H

#include "pass.h"

namespace lava::back {

/*
  this pass will generate prologue/epilogue for functions when necessary
*/
class FunctionFix : public PassBase {
private:
  int                   _gap;
  LLBlockPtr            _ret_block;
  LLInstList::iterator  _ret_pos;
  std::vector<ArmReg>   _saved_regs;


public:
  explicit FunctionFix(LLModule &module)
    : PassBase(module) {}

  void Reset() final {
    _saved_regs.clear();
    _gap = -1;
  }

  void AddPrologue(const LLFunctionPtr &func);

  void AddEpilogue(const LLFunctionPtr &func);

  void runOn(const LLFunctionPtr &func) final;
};

}


#endif //LAVA_FUNCFIX_H
