#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

#include "opt/analysis/dominance.h"

int Mem2Reg;

namespace lava::opt {

/*
 generate pure SSA form
 1. insert phi function
 2. rename each definition
 */
class Mem2Reg : public FunctionPass {
private:
  bool                                    _changed;
  DomInfo                                 _dom_info;

  std::vector<Value *>                    _allocas;
  std::unordered_map<Value *, uint32_t>   _alloca_ids;
  std::vector<std::vector<BasicBlock *>>  _alloca_defs;

  std::vector<BasicBlock *>               _worklist;

  std::unordered_map<std::shared_ptr<PhiNode>, uint32_t>    _phi_nodes; // map phinode with alloca instruction

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;
    CollectAlloca(F);
    CollectStore(F);
    PlacePhiNode(F);
    Rename(F);
    return _changed;
  }

  void initialize() final {
    auto dominance = PassManager::GetAnalysis<DominanceInfo>("DominanceInfo");
   _dom_info = dominance->GetDomInfo();
  }

  void finalize() final {
    _dom_info.clear();
    _allocas.clear();
    _alloca_ids.clear();
    _alloca_defs.clear();
    _worklist.clear();
    _phi_nodes.clear();
  }

  // collect all alloca instruction
  void CollectAlloca(const FuncPtr &F) {
    for (const auto &it : *F) {
      auto BB = dyn_cast<BasicBlock>(it.value());
      for (const auto &inst : BB->insts()) {
        if (auto alloc_inst = dyn_cast<AllocaInst>(inst)) {
          auto type = alloc_inst->type();
          DBG_ASSERT(type->IsPointer(), "type of alloca instruction is not pointer");
          if (type->GetDerefedType()->IsInteger()) {
            _alloca_ids.insert({alloc_inst.get(), (uint32_t)_alloca_ids.size()});
            _allocas.push_back(alloc_inst.get());
          }
        }
      }
    }
  }

  // collect all store instructions and match them up with alloca instructions
  void CollectStore(const FuncPtr &F) {
    DBG_ASSERT(_alloca_defs.empty(), "_alloca_defs is not empty");
    _alloca_defs = std::vector<std::vector<BasicBlock *>>(_alloca_ids.size());
    for (const auto &it : *F) {
      auto BB = dyn_cast<BasicBlock>(it.value());
      for (const auto &inst : BB->insts()) {
        if (auto store_inst = dyn_cast<StoreInst>(inst)) {
          auto res = _alloca_ids.find(store_inst->pointer().get());
          if (res != _alloca_ids.end()) {
            _alloca_defs[res->second].push_back(BB.get());
          }
        }
      }
    }
  }

  // 1. place phi node
  void PlacePhiNode(const FuncPtr &F) {
    std::unordered_set<BasicBlock *> visited;
    for (uint32_t id = 0; id < _allocas.size(); id++) {
      visited.clear();
      for (const auto &BB : _alloca_defs[id]) {
        _worklist.push_back(BB);
      }

      while (!_worklist.empty()) {
        BasicBlock *back = _worklist.back();
        _worklist.pop_back();

        // traverse its dominance frontier
        for (auto dom_frontier : _dom_info[F.get()].DF[back]) {
          if (visited.find(dom_frontier) == visited.end()) {
            visited.insert(dom_frontier);
            // create a phi node
            auto phi_node = std::make_shared<PhiNode>(dom_frontier);
            phi_node->Reserve();

            // insert this phi node to the head of dom_frontier
            auto begin = dom_frontier->insts().begin();
            dom_frontier->insts().insert(begin, phi_node);

            auto type = _allocas[id]->type();
            DBG_ASSERT(type->IsPointer(), "type of alloca instruction is not pointer");
            phi_node->set_type(type->GetDerefedType());

            // record the phi node
            _phi_nodes.insert({phi_node, id});

            // add dom_frontier into worklist
            _worklist.push_back(dom_frontier);
          }
        }
      }
    }
  }

  // 2. rename variables, replace load/store with move
  void Rename(const FuncPtr &F) {
    auto undef = std::make_shared<UnDefineValue>();
    auto entry = dyn_cast<BasicBlock>(F->entry()).get();
    std::unordered_set<BasicBlock *> visited;
    std::vector<std::pair<BasicBlock *, std::vector<SSAPtr>>> worklist{
        {entry, std::vector<SSAPtr>(_alloca_ids.size(), undef)}
    };


    while (!worklist.empty()) {
      BasicBlock *BB = worklist.back().first;
      std::vector<SSAPtr> values = std::move(worklist.back().second);
      worklist.pop_back();

      if (visited.find(BB) == visited.end()) {
        visited.insert(BB);
        for (auto it = BB->insts().begin(); it != BB->insts().end();) {
          auto next = std::next(it);

          if (auto res = _alloca_ids.find(it->get()); res != _alloca_ids.end()) {
            // remove from instruction list
            BB->insts().erase(it);
          } else if (auto load_inst = dyn_cast<LoadInst>(*it)) {
            // if alloc has been removed
            DBG_ASSERT(load_inst->Pointer() != nullptr, "pointer of load instruction is nullptr");
            auto alloc_it = _alloca_ids.find(load_inst->Pointer().get());
            if (alloc_it != _alloca_ids.end()) {
              load_inst->ReplaceBy(values[alloc_it->second]);
              BB->insts().erase(it);
            }
          } else if (auto store_inst = dyn_cast<StoreInst>(*it)) {
            auto alloc_it = _alloca_ids.find(store_inst->pointer().get());
            if (alloc_it != _alloca_ids.end()) {
              values[alloc_it->second] = store_inst->data();
              store_inst->RemoveValue(store_inst->data());
              BB->insts().erase(it);
            }
          } else if (auto phi_node = dyn_cast<PhiNode>(*it)) {
            auto phi_it = _phi_nodes.find(phi_node);
            if (phi_it != _phi_nodes.end()) {
              values[phi_it->second] = phi_node;
            }
          }

          it = next;
        }

        std::vector<BasicBlock *> succs;
        auto term_inst = *(--BB->insts().end());
        if (auto jump_inst = dyn_cast<JumpInst>(term_inst)) {
          succs.push_back(dyn_cast<BasicBlock>(jump_inst->target()).get());
        } else if (auto branch_inst = dyn_cast<BranchInst>(term_inst)) {
          succs.push_back(dyn_cast<BasicBlock>(branch_inst->true_block()).get());
          succs.push_back(dyn_cast<BasicBlock>(branch_inst->false_block()).get());
        } else if (auto ret_inst = dyn_cast<ReturnInst>(term_inst)) {
          // do nothing
        } else {
          ERROR("should not reach here");
        }

        for (auto &block : succs) {
          worklist.emplace_back(block, values);
          for (auto it = block->insts().begin(); it != block->insts().end(); it++) {
            if (auto phi_node = dyn_cast<PhiNode>(*it)) {
              auto res = _phi_nodes.find(phi_node);
              if (res != _phi_nodes.end()) {
                uint32_t idx = 0;
                for (; idx < block->size(); idx++) {
                  if ((*block)[idx].value().get() == BB) break;
                }
                phi_node->SetOperand(idx, values[res->second]);
              }
            } else {
              break;
            }
          }
        }
      }
    }

    for (const auto &it : _allocas) {
      it->ReplaceBy(nullptr);
    }
  }


};

class Mem2RegFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<Mem2Reg>();
    auto passinfo = std::make_shared<PassInfo>(pass, "Mem2Reg", false, false, MEMORY_TO_REGISTER);

    passinfo->Requires("DominanceInfo");

    return passinfo;
  }
};

static PassRegisterFactory<Mem2RegFactory> registry;

}