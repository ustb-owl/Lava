#include "loopinfo.h"

int LoopInfo;

namespace lava::opt {

bool LoopInfoPass::runOnFunction(const FuncPtr &F) {
  if (F->is_decl()) return false;
  _cur_func = F.get();
  auto entry = dyn_cast<BasicBlock>(F->entry());
  CollectLoops(entry.get());
  return false;
}

void LoopInfoPass::CollectLoops(BasicBlock *header) {
  TRACE("%lu\n", _dom_info[_cur_func].doms[header].size());
  DBG_ASSERT(_cur_func != nullptr, "current function is nullptr");
  for (auto &dominatee : _dom_info[_cur_func].doms[header]) {
    if (dominatee && dominatee != header) CollectLoops(dominatee);
  }

  std::vector<BasicBlock *> worklist;
  // exist edge from this block to header
  for (auto &pred : *header) {
    auto BB = dyn_cast<BasicBlock>(pred.value()).get();
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
          for (auto &BB : *pred) {
            auto block = dyn_cast<BasicBlock>(BB.value());
            worklist.push_back(block.get());
          }
        }
      } else {

        auto sub_loop = it->second;
        while (auto parent = sub_loop->parent()) { sub_loop = parent; }
        if (sub_loop != loop) {
          sub_loop->set_parent(loop);
          for (auto &BB : *(sub_loop->header())) {
            auto P = dyn_cast<BasicBlock>(BB.value());
            auto res = _loop_info.loop_of_bb().find(P.get());
            if (res == _loop_info.loop_of_bb().end() || res->second != sub_loop) {
              worklist.push_back(P.get());
            }
          }
        }
      }

    }

  }
}

static PassRegisterFactory<LoopInfoPassFactory> registry;
}