#include "loopinfo.h"

#include "opt/register.h"

int LoopInfo;

namespace lava::opt {

bool LoopInfoPass::runOnFunction(const FuncPtr &F) {
  if (F->is_decl())
    return false;
  _cur_func  = F.get();
  auto entry = F->entry();
  CollectLoops(entry.get());
  Populate(entry.get());
  return false;
}

void LoopInfoPass::CollectLoops(BasicBlock *header) {
  DBG_ASSERT(_cur_func != nullptr, "current function is nullptr");
  for (auto &dominatee : _dom_info[_cur_func].doms[header]) {
    if (dominatee != header)
      CollectLoops(dominatee);
  }

  std::vector<BasicBlock *> worklist;
  // exist edge from this block to header
  for (const auto &pred : header->predecessors()) {
    auto  BB    = pred.get();
    auto &domBy = _dom_info[_cur_func].domBy[BB];
    if (domBy.find(header) != domBy.end()) {
      worklist.push_back(BB);
    }
  }

  if (!worklist.empty()) {
    auto loop = std::make_shared<Loop>(header);
    while (!worklist.empty()) {
      auto pred = worklist.back();
      worklist.pop_back();
      auto [it, inserted] = _loop_info.loop_of_bb().insert({pred, loop});
      if (inserted) {
        if (pred != header) {
          for (const auto &BB : pred->predecessors()) {
            worklist.push_back(BB.get());
          }
        }
      } else {
        auto sub_loop = it->second;
        while (auto parent = sub_loop->getParent()) {
          sub_loop = parent;
        }
        if (sub_loop != loop) {
          sub_loop->setParent(loop);
          for (const auto &BB : sub_loop->header()->predecessors()) {
            auto P   = BB.get();
            auto res = _loop_info.loop_of_bb().find(P);
            if (res == _loop_info.loop_of_bb().end() ||
                res->second != sub_loop) {
              worklist.push_back(P);
            }
          }
        }
      }
    }
  }
}

void LoopInfoPass::Populate(BasicBlock *header) {
  if (_visited.find(header) != _visited.end())
    return;
  _visited.insert(header);

  for (auto &succ : header->successors()) {
    Populate(succ);
  }

  auto it       = _loop_info.loop_of_bb().find(header);
  auto sub_loop = (it == _loop_info.loop_of_bb().end()) ? nullptr : it->second;
  if (sub_loop && (sub_loop->header() == header)) {
    (sub_loop->getParent() ? sub_loop->getParent()->sub_loops()
                           : _loop_info.top_level())
        .push_back(sub_loop);
    std::reverse(sub_loop->blocks().begin() + 1, sub_loop->blocks().end());
    std::reverse(sub_loop->sub_loops().begin(), sub_loop->sub_loops().end());
  }
  for (; sub_loop; sub_loop = sub_loop->getParent())
    sub_loop->blocks().push_back(header);
}

void RegisterLoopInfoPass() {
  RegisterPassCliMetadata({
      "LoopInfoPass",
      "loop-info",
      {},
      "compute natural loop information",
  });
  static PassRegisterFactory<LoopInfoPassFactory> registry;
}
} // namespace lava::opt
