#include "liveness.h"
#include "common/casting.h"
#include <iostream>

namespace lava::back {


void LivenessAnalysis::runOn(const LLFunctionPtr &func) {
  if (func->is_decl()) return;
  Init(func);
  SolveLiveness();
  DumpInitInfo();

}

void LivenessAnalysis::Init(const LLFunctionPtr &F) {
  _rpo_blocks.clear();
  _visited.clear();
  _blk_info.clear();

  // get reverse post order block list
  TraverseRPO(F->entry());
  DBG_ASSERT(F->blocks().size() == _rpo_blocks.size(), "blocks size is different");

  // mark each block with id
  std::size_t id = 0;
  for (const auto &it : _rpo_blocks) {
    _blk_map.insert({id++, it});
  }

  for (const auto &it : _rpo_blocks) {
    OprSet ue_var, var_kill, live_out;
    for (const auto &inst : it->insts()) {
      // record kill info
      auto dst = inst->dest();
      if (dst != nullptr && dst->IsVirtual()) {
        var_kill.insert(dst);
      }

      // record gen info
      for (const auto &opr : inst->operands()) {
        if (opr && opr->IsVirtual() && !var_kill.count(opr)) {
          ue_var.insert(opr);
        }
      }

    }

    BlockInfo blk_info(ue_var, var_kill, live_out);
    _blk_info.insert({it, blk_info});
  }

}

void LivenessAnalysis::TraverseRPO(const LLBlockPtr &BB) {
  // return if visited
  if (!_visited.insert(BB).second) return;

  auto termInst = BB->insts().back();
  if (auto jump_inst = dyn_cast<LLJump>(termInst)) {
    // visit its successor
    TraverseRPO(jump_inst->target());
  } else if (auto branch_inst = dyn_cast<LLBranch>(termInst)) {
    // visit false block firstly
    TraverseRPO(branch_inst->true_block());
    TraverseRPO(branch_inst->false_block());
  } else if (auto ret_inst = dyn_cast<LLReturn>(termInst)) {
    // do nothing
  } else {
    ERROR("should not reach here");
  }
  _rpo_blocks.push_front(BB);
}

void LivenessAnalysis::DumpInitInfo() {
  for (const auto &it : _blk_info) {
    std::cout << it.first->name() << std::endl;

    // output ue
    std::cout << "ue: ";
    for (const auto &ue : it.second.ue_var) std::cout << ue << " ";
    std::cout << std::endl;

    // output kill
    std::cout << "kill: ";
    for (const auto &kill : it.second.var_kill) std::cout << kill << " ";
    std::cout << std::endl;

    std::cout << "liveout: ";
    for (const auto &liveout : it.second.live_out) std::cout << liveout << " ";
    std::cout << std::endl << std::endl;
  }
}

void LivenessAnalysis::SolveLiveness() {
  // clear live out set
  for(const auto &BB : _rpo_blocks) _blk_info[BB].live_out.clear();


  bool changed = true;
  while(changed) {
    changed = false;
    for (const auto &BB : _rpo_blocks) {
      auto &liveout = _blk_info[BB].live_out;
      auto succs = GetSuccessors(BB);

      // LiveOut(j) ← 􏰛k∈succ(j) UEVar(k) ∪ (LiveOut(k) ∩ VarKill(k))
      for (const auto &succ : succs) {
        auto &succ_info = _blk_info[succ];
        for (const auto &vreg : succ_info.ue_var) {
          if (liveout.insert(vreg).second) changed = true;
        }

        for (const auto &vreg : succ_info.live_out) {
          if (!succ_info.var_kill.count(vreg)) {
            if (liveout.insert(vreg).second) changed = true;
          }
        }
      }
    }
  }
}

std::vector<LLBlockPtr> LivenessAnalysis::GetSuccessors(const LLBlockPtr &BB) {
  std::vector<LLBlockPtr> succs;
  auto term_inst = BB->insts().back();
  if (auto jump_inst = dyn_cast<LLJump>(term_inst)) {
    succs.push_back(jump_inst->target());
  } else if (auto branch_inst = dyn_cast<LLBranch>(term_inst)) {
    succs.push_back(branch_inst->true_block());
    succs.push_back(branch_inst->false_block());
  } else if (auto ret_inst = dyn_cast<LLReturn>(term_inst)) {
    // do nothing
  } else {
    ERROR("should not reach here");
  }
  return succs;
}

}