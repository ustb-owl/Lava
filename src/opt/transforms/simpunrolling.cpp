#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

#include "opt/analysis/loopinfo.h"
#include "opt/transforms/gvn_gcm.h"

int LoopUnrolling;

namespace lava::opt {

/*
 try to unrolling const loop
 */
class SimpleUnrolling : public FunctionPass {
private:
  bool _changed;
  LoopInfo _loop_info;
  std::unordered_map<SSAPtr, SSAPtr> _ssa_map;
  std::unordered_map<SSAPtr, SSAPtr> _phi_map;
  std::unordered_map<SSAPtr, SSAPtr> _phi_tmp_map;
  std::unordered_set<SSAPtr> _phi_update;


public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    Unroll(F);
    auto A = PassManager::GetTransformPass<GlobalValueNumberingGlobalCodeMotion>("GlobalValueNumberingGlobalCodeMotion");
    A->initialize();
    A->runOnFunction(F);
    A->finalize();

    return _changed;
  }

  void initialize() final {
  }

  void finalize() final {}

  void Unroll(const FuncPtr &F) {
    auto A = PassManager::GetAnalysis<LoopInfoPass>("LoopInfoPass");
    A->initialize();
    A->runOnFunction(F);
    _loop_info = A->GetLoopInfo();

    auto deepest = _loop_info.deepest_loops();
    for (const auto &loop : deepest) {
      int i_init, end_num;
      SSAPtr induct_var = nullptr;
      SSAPtr update_stmt = nullptr;
      auto header = loop->header();
      if (CheckConstLoop(loop, i_init, end_num, induct_var, update_stmt)) {
        auto branch_inst = dyn_cast<BranchInst>(header->insts().back());
        auto cmp_inst = dyn_cast<ICmpInst>(branch_inst->cond());
        int loop_time = 0;

        auto bin_update_stmt = dyn_cast<BinaryOperator>(update_stmt);
        if (i_init < end_num && (bin_update_stmt->opcode() == BinaryOperator::BinaryOps::Add) &&
            (cmp_inst->op() == front::Operator::SLessEq || cmp_inst->op() == front::Operator::SLess)) {
          if (cmp_inst->op() == front::Operator::SLessEq) end_num += 1;
          int update_num = dyn_cast<ConstantInt>(bin_update_stmt->RHS())->value();
          loop_time = (end_num - i_init) / update_num;

          if (loop_time * loop->blocks()[1]->insts().size() > 200) { return; }

          if (((end_num - i_init) % update_num) == 0) {
            TRACE0();
            auto loop_body = loop->blocks()[1];

            // collect phi nodes
            for (const auto &inst : header->insts()) {
              if (auto phi_node = dyn_cast<PhiNode>(inst)) {
                _phi_map.insert({phi_node, (*phi_node)[1].value()});
                _phi_tmp_map.insert({phi_node, nullptr});
                _phi_update.insert((*phi_node)[1].value());
              }
            }

            auto loop_end_block = branch_inst->false_block();
            auto br = loop_body->insts().back();
            loop_body->insts().pop_back();  // remove br to while_cond;

            // save last jump instruction
            auto last_inst = loop_body->insts().back();

            // copy instructions
            for (auto i = 0; i < (loop_time - 1); i++) {
              for (auto it = loop_body->insts().begin();; it++) {
                CloneInstruction(*it, loop_body);
                if (*it == last_inst) break;
              }
              for (auto &it : _phi_map) {
                DBG_ASSERT(_phi_tmp_map.find(it.first) != _phi_tmp_map.end(), "failed phi-node failed");
                it.second = _phi_tmp_map[it.first];
              }
            }

            // replace first body with first value of phi
//#if 0
            for (auto it = loop_body->insts().begin();; it++) {
              auto inst = dyn_cast<Instruction>(*it);
              for (auto &use : *inst) {
                auto value = use.value();
                auto res = _phi_map.find(value);
                if (res != _phi_map.end()) {
                  auto phi = dyn_cast<PhiNode>(res->first);
                  use.set((*phi)[0].value());
                }
              }
              if (*it == last_inst) break;
            }
//#endif

            // replace phi-node's value with latest value
            for (auto &[k, v] : _phi_map) {
              auto phi = dyn_cast<PhiNode>(k);
              phi->RemoveValue((*phi)[1].value());
              phi->AddValue(v);
            }

            loop_body->insts().push_back(br);

            _ssa_map.clear();
            _phi_map.clear();
          }
        }

      }


    }

    A->finalize();
  }

  bool CheckConstLoop(const Loop *loop, int &i, int &end_num, SSAPtr &induction_var, SSAPtr &update_stmt) {
    auto header = loop->header();
    if (loop->blocks().size() != 3 || loop->blocks().back() != header) return false;
    auto term_inst = dyn_cast<BranchInst>(header->insts().back());
    DBG_ASSERT(term_inst != nullptr, "the last instruction of loop-condition should be branch");
    auto cond = dyn_cast<ICmpInst>(term_inst->cond());
    if (cond) {
      SSAPtr induct_var = nullptr;
      if (auto c1 = dyn_cast<ConstantInt>(cond->LHS())) {
        induct_var = cond->RHS();
        end_num = c1->value();
      } else if (auto c2 = dyn_cast<ConstantInt>(cond->RHS())) {
        induct_var = cond->LHS();
        end_num = c2->value();
      } else return false;
      if (auto phi_node = dyn_cast<PhiNode>(induct_var)) {
        if (phi_node->size() == 2) {
          if (auto init = dyn_cast<ConstantInt>((*phi_node)[0].value())) {
            auto new_i = (*phi_node)[1].value();
            if (auto binary_inst = dyn_cast<BinaryOperator>(new_i)) {
              if (binary_inst->LHS() == phi_node && IsSSA<ConstantInt>(binary_inst->RHS())) {

                // check loop body
                // return false if loop body has call instructions or local array
                int inst_cnt = 0;
                for (const auto &inst : loop->blocks()[1]->insts()) {
                  if (IsSSA<CallInst>(inst) || IsSSA<AllocaInst>(inst) || ++inst_cnt >= 16) return false;
                }

                i = init->value();
                induction_var = phi_node;
                update_stmt = binary_inst;
                return true;
              }
            }
          }
        }
      }
    }
    return false;
  }

  SSAPtr GetSSA(const SSAPtr &value) {
    SSAPtr res = nullptr;
    auto it = _ssa_map.find(value);
    if (it != _ssa_map.end()) res = it->second;
    else res = value;

    // replace with latest value
    if (auto phi = dyn_cast<PhiNode>(res)) {
      if (_phi_map.find(res) != _phi_map.end()) {
        auto latest_phi = _phi_map[res];
        res = latest_phi;
      }
    }

    return res;
  }

  void CloneInstruction(const SSAPtr &inst, BasicBlock *block) {
    SSAPtr result = nullptr;
    if (auto binary_inst = dyn_cast<BinaryOperator>(inst)) {
      result = BinaryOperator::Create(binary_inst->opcode(), GetSSA(binary_inst->LHS()), GetSSA(binary_inst->RHS()));
      DBG_ASSERT(result != nullptr, "copy binary instruction failed");
    } else if (auto access_inst = dyn_cast<AccessInst>(inst)) {
      SSAPtrList indexs;
      indexs.push_back(GetSSA(access_inst->index()));
      if (access_inst->has_multiplier()) indexs.push_back(access_inst->multiplier());
      result = std::make_shared<AccessInst>(access_inst->acc_type(), GetSSA(access_inst->ptr()), indexs);
      DBG_ASSERT(result != nullptr, "copy access instruction failed");
    } else if (auto load_inst = dyn_cast<LoadInst>(inst)) {
      result = std::make_shared<LoadInst>(GetSSA(load_inst->Pointer()));
      DBG_ASSERT(result != nullptr, "copy load instruction failed");
    } else if (auto store_inst = dyn_cast<StoreInst>(inst)) {
      result = std::make_shared<StoreInst>(GetSSA(store_inst->data()), GetSSA(store_inst->pointer()));
      DBG_ASSERT(result != nullptr, "copy store instruction failed");
    } else {
      ERROR("should not reach here");
    }

    if (_phi_update.find(inst) != _phi_update.end()) {
      for (auto &it : _phi_map) {
        auto phi = dyn_cast<PhiNode>(it.first);
        if ((*phi)[1].value() == inst) {
          _phi_tmp_map[phi] = result;
          break;
        }
      }
    }

    result->set_type(inst->type());
    block->insts().insert(block->insts().end(), result);
    _ssa_map.insert_or_assign(inst, result);
  }

};

class SimpleUnrollingFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<SimpleUnrolling>();
    auto passinfo = std::make_shared<PassInfo>(pass, "SimpleUnrolling", false, 2, LOOP_UNROLLING);


    return passinfo;
  }
};

static PassRegisterFactory<SimpleUnrollingFactory> registry;
}