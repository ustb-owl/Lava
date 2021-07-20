#ifndef LAVA_SPILL_H
#define LAVA_SPILL_H

#include "pass.h"

namespace lava::back {

class Spill : public PassBase {
private:
  std::uint32_t _mask;
  LLBlockPtr    _cur_block;
  LLFunctionPtr _cur_func;

public:
  explicit Spill(LLModule &module)
    : PassBase(module), _cur_block(nullptr) {}

  LLOperandPtr GetTmpReg(std::uint32_t &mask);

  std::uint32_t GetTmpMask(const LLInstPtr &inst);

  void InsertLoadInst(LLInstList::iterator &it,
                      const LLOperandPtr &slot, const LLOperandPtr &dst);

  void InsertStoreInst(LLInstList::iterator &it,
                       const LLOperandPtr &slot, const LLOperandPtr &tmp);

  void Reset() final {
    _mask = 0;
    _cur_block = nullptr;
    _cur_func = nullptr;
  }

  void runOn(const LLFunctionPtr &func) final;
};
}

#endif //LAVA_SPILL_H
