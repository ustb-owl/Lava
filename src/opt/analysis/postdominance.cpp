#include "postdominance.h"

int PostDominance;

namespace lava::opt {

void PostDominanceInfo::SolveDominance(const FuncPtr &F) {
  _cur_func = F.get();
  auto &info = _dom_info[_cur_func];
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto po = _blkWalker.POTraverse(entry.get());
  BasicBlock *exit = po.front();

  // init the dom set
  // set exit -> { exit }
  info.domBy[exit] = {exit};

  // store all basic blocks except entry
  std::unordered_set<BasicBlock *> all;

  for (auto &it : po) {
    all.insert(it);
  }

  // set Dom(i) <- all
  auto it = po.begin();
  for (it++; it != po.end(); it++) {
    info.domBy[*it] = all;
  }

  bool changed = true;
  while (true) {
    changed = false;

    // traverse all blocks except exit
    auto BB = po.begin();
    for (BB++; BB != po.end(); BB++) {
      auto block = *BB;
      // validate if the post-dominators exist in all of its successors
      for (auto post_dominator = info.domBy[block].begin(); post_dominator != info.domBy[block].end();) {
        auto successors = block->successors();
        if (*post_dominator != block &&
            std::any_of(successors.begin(), successors.end(), [&post_dominator, &info](BasicBlock *successor) {
              return info.domBy[successor].find(*post_dominator) == info.domBy[successor].end();
            })) {
          changed = true;
          post_dominator = info.domBy[block].erase(post_dominator);
        } else {
          post_dominator++;
        }
      }
    }

    if (!changed) break;
  }

  // check result
  DBG_ASSERT(info.domBy.size() == po.size(), "domBy size not equals to basic block numbers");

  // solve idom
  SolveImmediateDom();

  // solve DF
  SolveDominanceFrontier();
}

/*
 * The immediate dominator or idom of a node n is the unique node that strictly
 * dominates n but does not strictly dominate any other node that strictly dominates n.
 * Every node, except the entry node, has an immediate dominator.
 */
void PostDominanceInfo::SolveImmediateDom() {
  auto &info = _dom_info[_cur_func];
  auto entry = dyn_cast<BasicBlock>(_cur_func->entry()).get();
  auto po = _blkWalker.POTraverse(entry);
  BasicBlock *exit = po.front();

  // insert exit itself into its post-dominatee set
  info.doms[exit].insert(exit);

  auto it = po.begin();
  for (it++; it != po.end(); it++) {
    BasicBlock *BB = *it;

    // insert this BB into exit block's dominatees set
    info.doms[exit].insert(BB);

    // traverse BB's post-dominators
    for (BasicBlock *post_dominator : info.domBy[BB]) {
      // insert BB into its post-dominators dominatees set
      info.doms[post_dominator].insert(BB);

      // 1. check if post-dominator is strictly dominate BB
      if (post_dominator == BB) continue;

      // 2. check if post-dominator does not strictly dominate any other node
      // that strictly dominates BB
      auto sdoms = GetSDoms(BB);
      if (std::any_of(sdoms.begin(), sdoms.end(), [post_dominator, this](BasicBlock *sdominator) {
        return IsStrictlyDom(post_dominator, sdominator);
      })) { continue; }

      // set dominator as BB's  immediate dominator
      DBG_ASSERT(info.idom.find(BB) == info.idom.end(), "block already has a idom");
      info.idom[BB] = post_dominator;
    }
  }

  // check the result
  DBG_ASSERT(info.doms.size() == po.size(), "doms size not equals to basic block numbers");
  DBG_ASSERT(info.idom.size() == po.size() - 1, "idom set number is wrong");
  DBG_ASSERT(info.idom.find(exit) == info.idom.end(), "entry should not have idom");
}

void PostDominanceInfo::SolveDominanceFrontier() {
  auto &info = _dom_info[_cur_func];
  auto entry = dyn_cast<BasicBlock>(_cur_func->entry()).get();
  auto po = _blkWalker.POTraverse(entry);

  // set DF of all nodes as phi
  for (const auto &it : po) {
    info.DF[it].clear();
  }

  for (const auto &BB : po) {
    auto successors = BB->successors();
    if (successors.size() > 1) {
      for (const auto &succ : successors) {
        auto succ_block = static_cast<BasicBlock *>(succ);
        auto runner = succ_block;
        while (runner != info.idom[BB]) {
          info.DF[runner].insert(BB);
          runner = info.idom[runner];
        }
      }
    }
  }
}

static PassRegisterFactory<PostDominanceInfoPassFactory> registry;

}