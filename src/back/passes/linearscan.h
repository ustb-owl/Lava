#ifndef LAVA_LINEARSCAN_H
#define LAVA_LINEARSCAN_H

#include <map>
#include <utility>

#include "pass.h"
#include "liveness.h"
#include "back/slot.h"

namespace lava::back {


class LinearScanRegisterAllocation : public PassBase {
private:
  SlotAllocator                     _slot;
  std::shared_ptr<LivenessAnalysis> _liveness;

  LLOperandList _free_tmp_regs;
  LLOperandList _free_comm_regs;
  LLOperandList _free_slots;

  std::unordered_map<LLOperandPtr, LLOperandPtr> _alloc_map;
  std::map<LiveInterval, LLOperandPtr, CmpEnd>   _active;
  std::map<LiveInterval, LLOperandPtr, CmpStart> _live_intervals;

public:
  explicit LinearScanRegisterAllocation(LLModule &module, std::shared_ptr<LivenessAnalysis> liveness)
      : PassBase(module), _liveness(std::move(liveness)) {}

  void Initialize() final;

  void Reset() final {
    _alloc_map.clear();
    _free_tmp_regs.clear();
    _free_comm_regs.clear();
    _free_slots.clear();
    _active.clear();
    _live_intervals.clear();
  }

  void SolveLinearScanRegisterAllocation(const LLFunctionPtr &func);

  void ExpireOldIntervals(const LiveInterval &live_interval);

  void SpillAtInterval(const LiveInterval &live_interval, const LLFunctionPtr &func);

  void runOn(const LLFunctionPtr &func) final;
};
}

#endif //LAVA_LINEARSCAN_H
