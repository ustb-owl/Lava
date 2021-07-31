#include "linearscan.h"

namespace lava::back {

void LinearScanRegisterAllocation::Initialize() {
  _alloc_map.clear();

  _free_tmp_regs.push_back(LLOperand::Register(ArmReg::r0));
  _free_tmp_regs.push_back(LLOperand::Register(ArmReg::r1));
  _free_tmp_regs.push_back(LLOperand::Register(ArmReg::r2));
  _free_tmp_regs.push_back(LLOperand::Register(ArmReg::r3));

  for (auto reg = int(ArmReg::r4); reg <= int(ArmReg::r10); reg++) {
    _free_comm_regs.push_back(LLOperand::Register(ArmReg(reg)));
  }

  _free_slots.clear();
}

void LinearScanRegisterAllocation::runOn(const LLFunctionPtr &func) {
  _liveness->Initialize();
  _liveness->runOn(func);

  auto &live_intervals = _liveness->GetLiveInterval();
  // build live intervals which ordered by start position
  for (const auto &it : live_intervals) {
    _live_intervals.insert({it.second, it.first});
  }
  DBG_ASSERT(_live_intervals.size() == live_intervals.size(), "size of _live_intervals is wrong");

  SolveLinearScanRegisterAllocation(func);

  // set allocated result
  for (const auto &BB : func->blocks()) {
    for (const auto &inst : BB->insts()) {
      for (const auto &opr : inst->operands()) {
        if (opr && opr->IsVirtual()) {
          DBG_ASSERT(_alloc_map.find(opr) != _alloc_map.end(), "can't find the vreg's allocated result");
          auto alloc_result = _alloc_map[opr];

          // add to function saved set
          if (alloc_result->IsRealReg() && !IsTempReg(alloc_result)) {
            func->AddSavedRegister(alloc_result->reg());
          }

          opr->set_allocated(alloc_result);
        }
      }

      auto dst = inst->dest();
      if (dst && dst->IsVirtual()) {
        DBG_ASSERT(_alloc_map.find(dst) != _alloc_map.end(), "can't find the vreg's allocated result");
        auto alloc_result = _alloc_map[dst];

        // add to function saved set
        if (alloc_result->IsRealReg() && !IsTempReg(alloc_result)) {
          func->AddSavedRegister(alloc_result->reg());
        }

        dst->set_allocated(alloc_result);
      }
    }
  }

  _liveness->Reset();
}

void LinearScanRegisterAllocation::SolveLinearScanRegisterAllocation(const LLFunctionPtr &func) {
  DBG_ASSERT(_active.empty(), "active set is not empty");
  for (const auto &it : _live_intervals) {
    ExpireOldIntervals(it.first);

    if (it.first.can_alloc_to_tmp() && !_free_tmp_regs.empty()) {
      // alloc a temp register
      auto tmp = _free_tmp_regs.back();
      _free_tmp_regs.pop_back();
      _alloc_map.insert({it.second, tmp});
      _active.insert({it.first, tmp});
    } else if (!_free_comm_regs.empty()) {
      // alloc a common register
      auto tmp = _free_comm_regs.back();
      _free_comm_regs.pop_back();
      _alloc_map.insert({it.second, tmp});
      _active.insert({it.first, tmp});
    } else if (!_free_slots.empty()) {
      // alloc a free slot
      auto tmp = _free_slots.back();
      _free_slots.pop_back();
      _alloc_map.insert({it.second, tmp});
      _active.insert({it.first, tmp});
    } else {
      SpillAtInterval(it.first, func);
    }
  }
}

void LinearScanRegisterAllocation::ExpireOldIntervals(const LiveInterval &live_interval) {
  for (auto it = _active.begin(); it != _active.end();) {
    if ((*it).first.end_pos() >= live_interval.start_pos()) return;
    auto opr = it->second;
    if (IsTempReg(opr)) {
      _free_tmp_regs.push_back(opr);
    } else if (it->second->IsRealReg()) {
      _free_comm_regs.push_back(opr);
    } else {
      DBG_ASSERT(it->second->IsImmediate(), "this operand is not a slot");
      _free_slots.push_back(opr);
    }
    it = _active.erase(it);
  }
}

void LinearScanRegisterAllocation::SpillAtInterval(const LiveInterval &live_interval, const LLFunctionPtr &func) {
  auto end = std::prev(_active.end());
  auto spill = *end;

  // _alloc_map[live_interval] = _alloc_map[spill]
  if (spill.first.end_pos() > live_interval.end_pos()) {
    auto vreg = _live_intervals[spill.first];
    auto opr = _alloc_map[vreg];

    auto vreg_new = _live_intervals[live_interval];
    _alloc_map[vreg_new] = opr;
    _alloc_map[vreg] = _slot.AllocSlot(func, vreg);
  } else {
    auto vreg = _live_intervals[live_interval];
    auto slot = _slot.AllocSlot(func, vreg);
    _alloc_map[vreg] = slot;
  }
}

}