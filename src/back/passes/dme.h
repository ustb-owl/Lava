#ifndef LAVA_DME_H
#define LAVA_DME_H

#include "pass.h"

namespace lava::back {

/*
 this pass will eliminate dead move instruction
 mov r1, r0
 ...
 mov r2, r1 => mov r2, r0
 */
class DeadMoveElimination : public PassBase {
private:
  std::unordered_set<LLOperandPtr> _visited;
  std::unordered_map<LLOperandPtr, std::pair<LLBlockPtr, LLInstList::iterator>> _mov_dst_map;

public:
  explicit DeadMoveElimination(LLModule &module) : PassBase(module) {}

  void Reset() final {}

  void runOn(const LLFunctionPtr &func) final;
};
}

#endif //LAVA_DME_H
