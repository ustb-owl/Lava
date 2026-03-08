#include "common/casting.h"
#include "opt/analysis/dominance.h"
#include "opt/pass.h"
#include "opt/pass_manager.h"
#include "opt/register.h"

int Mem2Reg;

namespace lava::opt {

/*
 generate pure SSA form
 1. insert phi function
 2. rename each definition
 */
class Mem2Reg : public FunctionPass {
private:
  bool _changed;

  std::vector<Value *>                   _allocas;
  std::unordered_map<Value *, uint32_t>  _alloca_ids;
  std::vector<std::vector<BasicBlock *>> _alloca_defs;

  std::vector<BasicBlock *> _worklist;

  std::unordered_map<std::shared_ptr<PhiNode>, uint32_t>
      _phi_nodes; // map phinode with alloca instruction

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl())
      return _changed;
    //    if (F->GetFunctionName() == "median") return _changed;
    CollectAlloca(F);
    CollectStore(F);
    PlacePhiNode(F);
    Rename(F);
    return _changed;
  }

  void initialize() final {}

  void finalize() final {
    //    _dom_info.clear();
    _allocas.clear();
    _alloca_ids.clear();
    _alloca_defs.clear();
    _worklist.clear();
    _phi_nodes.clear();
  }

  // collect all alloca instruction
  void CollectAlloca(const FuncPtr &F) {
    for (const auto &it : *F) {
      for (const auto &inst : it->insts()) {
        if (auto alloc_inst = dyn_cast<AllocaInst>(inst)) {
          auto type = alloc_inst->type();
          DBG_ASSERT(type->IsPointer(),
                     "type of alloca instruction is not pointer");
          if (type->GetDerefedType()->IsInteger() ||
              type->GetDerefedType()->IsPointer()) {
            _alloca_ids.insert(
                {alloc_inst.get(), (uint32_t)_alloca_ids.size()});
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
      for (const auto &inst : it->insts()) {
        if (auto store_inst = dyn_cast<StoreInst>(inst)) {
          auto res = _alloca_ids.find(store_inst->pointer().get());
          if (res != _alloca_ids.end()) {
            _alloca_defs[res->second].push_back(it.get());
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
        const auto &dom_info =
            PassManager::RequireAnalysisResult<DomInfo>("DominanceInfo");
        auto func_it = dom_info.find(F.get());
        DBG_ASSERT(func_it != dom_info.end(), "dominance info is missing");
        auto frontier_it = func_it->second.DF.find(back);
        if (frontier_it == func_it->second.DF.end())
          continue;

        // traverse its dominance frontier
        for (auto dom_frontier : frontier_it->second) {
          if (visited.find(dom_frontier) == visited.end()) {
            visited.insert(dom_frontier);
            // create a phi node
            auto phi_node = std::make_shared<PhiNode>(dom_frontier);

            // insert this phi node to the head of dom_frontier
            dom_frontier->InsertInst(dom_frontier->inst_begin(), phi_node);

            auto type = _allocas[id]->type();
            DBG_ASSERT(type->IsPointer(),
                       "type of alloca instruction is not pointer");
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
    undef->set_type(MakePrimType(Type::Int32, true));
    auto                             entry = F->entry().get();
    std::unordered_set<BasicBlock *> visited;
    std::vector<std::pair<BasicBlock *, std::vector<SSAPtr>>> worklist{
        {entry, std::vector<SSAPtr>(_alloca_ids.size(), undef)}};

    std::vector<InstPtr> allocas_to_remove;

    while (!worklist.empty()) {
      BasicBlock         *BB     = worklist.back().first;
      std::vector<SSAPtr> values = std::move(worklist.back().second);
      worklist.pop_back();

      if (visited.find(BB) == visited.end()) {
        visited.insert(BB);
        for (auto it = BB->insts().begin(); it != BB->insts().end();) {
          auto next = std::next(it);

          if (auto res = _alloca_ids.find(it->get());
              res != _alloca_ids.end()) {
            // remove from instruction list
            allocas_to_remove.push_back(*it);
          } else if (auto load_inst = dyn_cast<LoadInst>(*it)) {
            // if alloc has been removed
            DBG_ASSERT(load_inst->Pointer() != nullptr,
                       "pointer of load instruction is nullptr");
            auto alloc_it = _alloca_ids.find(load_inst->Pointer().get());
            if (alloc_it != _alloca_ids.end()) {
              auto target = values[alloc_it->second];
              if (target->classId() == ClassId::UnDefineValueId) {
                target->set_type(load_inst->type());
              }
              load_inst->ReplaceBy(values[alloc_it->second]);
              load_inst->EraseFromParent();
            }
          } else if (auto store_inst = dyn_cast<StoreInst>(*it)) {
            auto alloc_it = _alloca_ids.find(store_inst->pointer().get());
            if (alloc_it != _alloca_ids.end()) {
              values[alloc_it->second] = store_inst->data();
              store_inst->EraseFromParent();
            }
          } else if (auto phi_node = dyn_cast<PhiNode>(*it)) {
            auto phi_it = _phi_nodes.find(phi_node);
            if (phi_it != _phi_nodes.end()) {
              values[phi_it->second] = phi_node;
            }
          }

          it = next;
        }

        for (auto *block : BB->successors()) {
          worklist.emplace_back(block, values);
          for (auto it = block->insts().begin(); it != block->insts().end();
               it++) {
            if (auto phi_node = dyn_cast<PhiNode>(*it)) {
              auto res = _phi_nodes.find(phi_node);
              if (res != _phi_nodes.end()) {
                phi_node->setIncomingValue(BB, values[res->second]);
              }
            } else {
              break;
            }
          }
        }
      }
    }

    // remove from user
    for (const auto &it : _allocas) {
      it->RemoveFromUser();
    }

    // remove from instruction list
    for (const auto &alloca : allocas_to_remove) {
      auto inst = dyn_cast<Instruction>(alloca);
      DBG_ASSERT(inst != nullptr, "alloca is not an instruction");
      inst->EraseFromParent();
    }
  }
};

class Mem2RegFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass     = std::make_shared<Mem2Reg>();
    auto passinfo = std::make_shared<PassInfo>(pass, "Mem2Reg", false, 1,
                                               MEMORY_TO_REGISTER);

    passinfo->Requires("DominanceInfo");

    return passinfo;
  }
};

void RegisterMem2RegPass() {
  RegisterPassCliMetadata({
      "Mem2Reg",
      "mem2reg",
      {},
      "promote stack slots to SSA values",
  });
  static PassRegisterFactory<Mem2RegFactory> registry;
}

} // namespace lava::opt
