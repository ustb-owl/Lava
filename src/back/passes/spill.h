#ifndef LAVA_SPILL_H
#define LAVA_SPILL_H

#include "pass.h"

namespace lava::back {

class Spill : public PassBase {
private:
  LLBlockPtr _cur_block;

public:
  explicit Spill(LLModule &module)
    : PassBase(module), _cur_block(nullptr) {}

  LLOperandPtr GetTmpReg(std::uint32_t &mask);

  std::uint32_t GetTmpMask(const LLInstPtr &inst);

  void InsertLoadInst(LLInstList::iterator &it,
                      const LLOperandPtr &slot, const LLOperandPtr &dst);

  void InsertStoreInst(LLInstList::iterator &it,
                       const LLOperandPtr &slot, const LLOperandPtr &tmp);

  void Reset() final { _cur_block = nullptr; }

  void runOn(const LLFunctionPtr &func) final;
};
}

#endif //LAVA_SPILL_H
