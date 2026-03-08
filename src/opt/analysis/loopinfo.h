#ifndef LAVA_LOOPINFO_H
#define LAVA_LOOPINFO_H

#include "common/casting.h"
#include "opt/analysis/dominance.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"

namespace lava::opt {

class Loop;

using LoopPtr = std::shared_ptr<Loop>;

class Loop {
private:
  LoopPtr                   _parent;
  std::vector<LoopPtr>      _sub_loops;
  std::vector<BasicBlock *> _bbs;

public:
  explicit Loop(BasicBlock *head) : _parent(nullptr), _bbs{head} {}

  BasicBlock *header() const { return _bbs[0]; }

  LoopPtr getParent() { return _parent; }

  void setParent(const LoopPtr &p) { _parent = p; }

  int depth() const {
    int ret = 0;
    for (const Loop *x = this; x; x = x->_parent.get())
      ++ret;
    return ret;
  }

  void get_deepest_loops(std::vector<Loop *> &deepest) {
    if (_sub_loops.empty())
      deepest.push_back(this);
    else {
      for (const auto &loop : _sub_loops) {
        loop->get_deepest_loops(deepest);
      }
    }
  }

  std::vector<BasicBlock *> &blocks() { return _bbs; }

  const std::vector<BasicBlock *> &blocks() const { return _bbs; }

  std::vector<LoopPtr> &sub_loops() { return _sub_loops; }
};

class LoopInfo {
private:
  // deepest loop which this block located in
  std::unordered_map<BasicBlock *, LoopPtr> _loop_of_bb;
  std::vector<LoopPtr>                      _top_level;

public:
  // get depth of basic block
  int depth_of(BasicBlock *BB) const {
    auto it = _loop_of_bb.find(BB);
    return it == _loop_of_bb.end() ? 0 : it->second->depth();
  }

  std::unordered_map<BasicBlock *, LoopPtr> &loop_of_bb() {
    return _loop_of_bb;
  }

  std::vector<LoopPtr> &top_level() { return _top_level; }

  const std::vector<LoopPtr> &top_level() const { return _top_level; }

  std::vector<Loop *> deepest_loops() const {
    std::vector<Loop *> deepest;
    for (const auto &l : _top_level) {
      l->get_deepest_loops(deepest);
    }
    return deepest;
  }

  void Clear() {
    _loop_of_bb.clear();
    _top_level.clear();
  }
};

using LoopInfoMap = std::unordered_map<Function *, LoopInfo>;

class LoopInfoPass : public FunctionPass {
private:
  Function                        *_cur_func;
  std::unordered_set<BasicBlock *> _visited;

  LoopInfoMap &GetMutableLoopInfoMap() {
    return PassManager::GetMutableAnalysisResult<LoopInfoMap>(name());
  }

  const DominanceResult &CurrentDominance() const {
    DBG_ASSERT(_cur_func != nullptr, "current function is nullptr");
    return PassManager::GetAnalysisResult<DomInfo>("DominanceInfo")
        .at(_cur_func);
  }

public:
  void initialize() final {
    _cur_func = nullptr;
    _visited.clear();
    static_cast<void>(
        PassManager::RequireAnalysisResult<DomInfo>("DominanceInfo"));
  }

  void finalize() final { _visited.clear(); }

  void CollectLoops(BasicBlock *header);

  void Populate(BasicBlock *header);

  const LoopInfo &GetLoopInfo(Function *F) const {
    return PassManager::GetAnalysisResult<LoopInfoMap>(name()).at(F);
  }

  bool runOnFunction(const FuncPtr &F) final;
};

class LoopInfoPassFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<LoopInfoPass>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "LoopInfoPass", true, 0, LOOP_INFO);
    passinfo->Requires("DominanceInfo");
    return passinfo;
  }
};

} // namespace lava::opt

#endif // LAVA_LOOPINFO_H
