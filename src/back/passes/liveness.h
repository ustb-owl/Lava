#ifndef LAVA_LIVENESS_H
#define LAVA_LIVENESS_H

#include "pass.h"

namespace lava::back {

typedef std::unordered_set<LLOperandPtr> OprSet;

class BlockInfo {
public:
  BlockInfo() = default;
  BlockInfo(OprSet ue, OprSet kill, OprSet live) :
             ue_var(std::move(ue)), var_kill(std::move(kill)), live_out(std::move(live)) {}
  OprSet ue_var;
  OprSet var_kill;
  OprSet live_out;
};

class LiveInterval {
private:
  std::size_t _start_pos;
  std::size_t _end_pos;
  bool        _can_alloc_to_tmp;
public:
  LiveInterval() : _start_pos(0), _end_pos(0), _can_alloc_to_tmp(true) {}
  LiveInterval(std::size_t start, std::size_t end, bool value)
    : _start_pos(start), _end_pos(end), _can_alloc_to_tmp(value) {}

  std::size_t start_pos() const { return _start_pos;        }
  std::size_t end_pos()   const { return _end_pos;          }
  bool can_alloc_to_tmp() const { return _can_alloc_to_tmp; }

  void SetStartPos(std::size_t pos) { _start_pos = pos;          }
  void SetEndPos(std::size_t pos)   { _end_pos = pos;            }
  void SetCanAllocTmp(bool value)   { _can_alloc_to_tmp = value; }
};

// methods for sort
bool LiveIntervalCmpStart(const LiveInterval &S1, const LiveInterval &S2);
bool LiveIntervalCmpEnd(const LiveInterval &S1, const LiveInterval &S2);
bool __CmpStart(const LiveInterval &S1, const LiveInterval &S2);
bool __CmpEnd(const LiveInterval &S1, const LiveInterval &S2);

struct CmpStart {
  bool operator()(const LiveInterval& S1, const LiveInterval &S2) const {
    return __CmpStart(S1, S2);
  }
};

struct CmpEnd {
  bool operator()(const LiveInterval& S1, const LiveInterval &S2) const {
    return __CmpEnd(S1, S2);
  }
};

class LivenessAnalysis : public PassBase {
private:
  std::list<LLBlockPtr>                           _rpo_blocks;
  std::unordered_set<LLBlockPtr>                  _visited;
  std::unordered_map<LLBlockPtr, BlockInfo>       _blk_info;
  std::unordered_map<std::size_t, LLBlockPtr>     _blk_map;
  std::unordered_map<LLOperandPtr, LiveInterval>  _live_intervals;

public:
  explicit LivenessAnalysis(LLModule &module) : PassBase(module) {}

  void Reset() final {
    _rpo_blocks.clear();
    _visited.clear();
    _blk_info.clear();
    _blk_map.clear();
    _live_intervals.clear();
  }

  // init gen and kill set
  void Init(const LLFunctionPtr &F);

  // traverse basic block in RPO
  void TraverseRPO(const LLBlockPtr &BB);

  // get successors of bb
  std::vector<LLBlockPtr> GetSuccessors(const LLBlockPtr &BB);

  // used for debug
  void DumpInitInfo(const LLFunctionPtr &func);

  // used for debug
  void DumpLiveInterval();

  // get live out information
  void SolveLiveness();

  // solve live interval
  void SolveLiveInterval(const LLFunctionPtr &func);

  // get live interval information
  const std::unordered_map<LLOperandPtr, LiveInterval> &
  GetLiveInterval() const {
    return _live_intervals;
  }

  // record live interval for each operands
  void RecordLiveInterval(const LLOperandPtr &opr, std::size_t end_pos, std::size_t last_tmp_pos);

  void runOn(const LLFunctionPtr &func) final;
};

// check if it is temp reg
bool IsTempReg(const LLOperandPtr &opr);

}

#endif //LAVA_LIVENESS_H
