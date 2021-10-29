#include "opt/pass.h"
#include "common/casting.h"
#include "opt/pass_manager.h"

#include "opt/analysis/loopinfo.h"
#include "opt/analysis/identiy.h"
#include "opt/transforms/gvn_gcm.h"

int LoopUnrolling;

namespace lava::opt {

/*
 try to unrolling const loop
 */
class SimpleUnrolling : public FunctionPass {
private:
  bool _changed;
  bool _in_last_loop;
  LoopInfo _loop_info;
  std::unordered_set<SSAPtr>         _phi_update;
  std::unordered_map<SSAPtr, SSAPtr> _ssa_map;
  std::unordered_map<SSAPtr, SSAPtr> _phi_map;
  std::unordered_map<SSAPtr, SSAPtr> _phi_tmp_map;
  std::unordered_map<SSAPtr, SSAPtr> _last_phi_map;
  std::unordered_map<SSAPtr, SSAPtr> _phi_origin_map;
  std::unordered_set<User *> _should_not_replace;

public:
  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    auto need_gcm = PassManager::GetAnalysis<NeedGcm>("NeedGcm");
    if (need_gcm->IsCrypto()) return _changed;
    TRACE0();

    auto A = PassManager::GetTransformPass<GlobalValueNumberingGlobalCodeMotion>("GlobalValueNumberingGlobalCodeMotion");
    _in_last_loop = false;
    UnrollConst(F);

    A->initialize();
    A->runOnFunction(F);
    A->finalize();

#if 0
    UnrollLeftConst(F);

    A->initialize();
    A->runOnFunction(F);
    A->finalize();
#endif

    return _changed;
  }

  void initialize() final {}

  void finalize() final {
    _phi_map.clear();
    _should_not_replace.clear();
    _phi_tmp_map.clear();
    _phi_update.clear();
    _ssa_map.clear();
    _last_phi_map.clear();
    _phi_origin_map.clear();
  }

  void UnrollConst(const FuncPtr &F) {
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

          if (loop_time * loop->blocks()[1]->insts().size() > 200) { continue; }

          if (((end_num - i_init) % update_num) == 0) {
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
            _phi_tmp_map.clear();
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

  void UnrollLeftConst(const FuncPtr &F) {
    auto A = PassManager::GetAnalysis<LoopInfoPass>("LoopInfoPass");
    A->initialize();
    A->runOnFunction(F);
    _loop_info = A->GetLoopInfo();

    auto deepest = _loop_info.deepest_loops();
    for (const auto &loop : deepest) {
      int i_init;
      SSAPtr induct_var = nullptr;
      SSAPtr update_stmt = nullptr;
      SSAPtr end_var = nullptr;
      auto header = loop->header();
      if (CheckLeftConstLoop(loop, i_init, end_var, induct_var, update_stmt)) {
        auto branch_inst = dyn_cast<BranchInst>(header->insts().back());
        auto cmp_inst = dyn_cast<ICmpInst>(branch_inst->cond());
        auto cmp_pos = header->inst_begin();
        auto bin_update_stmt = dyn_cast<BinaryOperator>(update_stmt);

        // continue if while end has multi-pred
        auto while_end = dyn_cast<BasicBlock>(branch_inst->false_block());
        if (while_end->size() > 1) continue;

        if ((bin_update_stmt->opcode() == BinaryOperator::BinaryOps::Add) &&
            (cmp_inst->op() == front::Operator::SLessEq || cmp_inst->op() == front::Operator::SLess)) {

          if (cmp_inst->op() == front::Operator::SLessEq) {
            i_init -= 1;
          }
          int update_num = dyn_cast<ConstantInt>(bin_update_stmt->RHS())->value();

          if (4 * loop->blocks()[1]->insts().size() > 200) { continue; }
          if (update_num != 1) continue;


          for (; cmp_pos != header->insts().end(); cmp_pos++) {
            if (*cmp_pos == cmp_inst) break;
          }
          DBG_ASSERT(*cmp_pos == cmp_inst, "find icmp instruction position failed");

          // unroll 2 times
          // TODO: for new c_x must be 1
          // for (i = 1; i < n; i = i + c_x)
          // => loop_time = (n - i) / (2 * c_x)
          // => rem = (n - i) % (2 * c_x)
          // => for (i = 1; i < loop_time; i + 2 * c_x)
          auto init_var = std::make_shared<ConstantInt>(i_init);
          init_var->set_type(MakePrimType(Type::Int32, true));
          auto tmp = BinaryOperator::Create(
              BinaryOperator::BinaryOps::Sub,
              end_var,
              init_var
          );
          tmp->set_type(init_var->type());

          auto size = std::make_shared<ConstantInt>(update_num * 2);
          size->set_type(MakePrimType(Type::Int32, true));

          auto count = BinaryOperator::Create(
              BinaryOperator::BinaryOps::SDiv,
              tmp,
              size
          );
          count->set_type(init_var->type());

          auto mult = BinaryOperator::Create(
              BinaryOperator::BinaryOps::Mul,
              count,
              size
          );
          mult->set_type(init_var->type());

          auto new_loop_end = BinaryOperator::Create(
              BinaryOperator::BinaryOps::Add,
              mult,
              init_var
          );
          new_loop_end->set_type(init_var->type());

          auto rem = BinaryOperator::Create(
              BinaryOperator::BinaryOps::Sub,
              tmp,
              mult
          );
          rem->set_type(init_var->type());

          cmp_pos = header->insts().insert(cmp_pos, rem);
          cmp_pos = header->insts().insert(cmp_pos, new_loop_end);
          cmp_pos = header->insts().insert(cmp_pos, mult);
          cmp_pos = header->insts().insert(cmp_pos, count);
          cmp_pos = header->insts().insert(cmp_pos, tmp);

          // replace compare condition
          (*cmp_inst)[1].set(new_loop_end);

          auto loop_body = loop->blocks()[1];


          /* handle last loop */
          auto check_rem_block = std::make_shared<BasicBlock>(F, "if.cond");
          auto last_loop_block = std::make_shared<BasicBlock>(F, "block");
          F->SetOperandNum(F->size() + 2);
          F->AddValue(check_rem_block);
          F->AddValue(last_loop_block);

          auto one = std::make_shared<ConstantInt>(1);
          one->set_type(init_var->type());
          auto cmp_rem = std::make_shared<ICmpInst>(
              front::Operator::NotEqual,
              rem,
              one
          );
          cmp_rem->set_type(MakePrimType(define::Type::Bool, false));

          auto branch_to_last = std::make_shared<BranchInst>(cmp_rem, while_end, last_loop_block);
          check_rem_block->insts().push_back(cmp_rem);
          check_rem_block->insts().push_back(branch_to_last);

          // connect while cond to rem_check
          (*branch_inst)[2].set(check_rem_block);

          // add header to check_rem 's pred
          check_rem_block->AddValue((*loop_body)[0].value());

          // remove header from while end
          while_end->RemoveValue(header);

          // renew while_end's predecessor
          while_end->AddValue(check_rem_block);
          while_end->AddValue(last_loop_block);


          // collect phi nodes
          for (const auto &inst : header->insts()) {
            if (auto phi_node = dyn_cast<PhiNode>(inst)) {
              _phi_map.insert({phi_node, (*phi_node)[1].value()});
              _phi_tmp_map.insert({phi_node, nullptr});
              _phi_origin_map.insert({phi_node, (*phi_node)[1].value()});
              _phi_update.insert((*phi_node)[1].value());
            }
          }

          auto loop_end_block = branch_inst->false_block();
          auto br = loop_body->insts().back();
          loop_body->insts().pop_back();  // remove br to while_cond;

          // save last jump instruction
          auto last_inst = loop_body->insts().back();

          // copy instruction in origin body
          for (auto it = loop_body->insts().begin();; it++) {
            CloneInstruction(*it, loop_body);
            if (*it == last_inst) break;
          }

          for (auto &it : _phi_map) {
            DBG_ASSERT(_phi_tmp_map.find(it.first) != _phi_tmp_map.end(), "failed phi-node failed");
            it.second = _phi_tmp_map[it.first];
          }

          // replace phi-node's value with latest value
          for (auto &[k, v] : _phi_map) {
            auto phi = dyn_cast<PhiNode>(k);
            phi->RemoveValue((*phi)[1].value());
            phi->AddValue(v);
          }

          // copy instruction in last block
          _in_last_loop = true;
          for (auto it = loop_body->insts().begin();; it++) {
            CloneInstruction(*it, last_loop_block.get());
            for (const auto &[k, v] : _phi_origin_map) {
              if (v == *it) {
                _last_phi_map.insert({k, last_loop_block->insts().back()});
              }
            }
            if (*it == last_inst) break;
          }
          DBG_ASSERT(_last_phi_map.size() == _phi_origin_map.size(), "last value number is wrong");
          _in_last_loop = false;

          loop_body->insts().push_back(br);

          // connect last body to while.end
          auto br_to_end = std::make_shared<JumpInst>(while_end);
          last_loop_block->insts().push_back(br_to_end);

          // insert phi nodes at while_end
          for (const auto &[k, v] : _phi_map) {
            DBG_ASSERT(_last_phi_map.find(k) != _last_phi_map.end(), "found last value of phi failed");
            SSAPtr last_value = _last_phi_map[k];
            auto phi = dyn_cast<PhiNode>(k);
            for (const auto &use : phi->uses()) {
              // find in last body
              auto res = std::find_if(last_loop_block->inst_begin(), last_loop_block->inst_end(),
                  [&use](const SSAPtr &inst){
                    return inst.get() == use->getUser();
                  }
              );
              if (res != last_loop_block->insts().end()) {
                _should_not_replace.insert(dyn_cast<User>((*res)).get());
              }

              // find in origin body
              res = std::find_if(loop_body->inst_begin(), loop_body->inst_end(),
                                      [&use](const SSAPtr &inst){
                                        return inst.get() == use->getUser();
                                      }
              );
              if (res != loop_body->insts().end()) {
                _should_not_replace.insert(dyn_cast<User>(*res).get());
              }

              // find in header
              res = std::find_if(header->inst_begin(), header->inst_end(),
                                 [&use](const SSAPtr &inst){
                                   return inst.get() == use->getUser();
                                 }
              );
              if (res != header->insts().end()) {
                _should_not_replace.insert(dyn_cast<User>(*res).get());
              }
            }
            DBG_ASSERT(last_value != nullptr, "find user of phi failed");
            auto new_phi = std::make_shared<PhiNode>(while_end.get());
            new_phi->Reserve();
            (*new_phi)[0].set(phi);
            (*new_phi)[1].set(last_value);
            new_phi->set_type(phi->type());
            while_end->insts().insert(while_end->insts().begin(), new_phi);
            _should_not_replace.insert(dyn_cast<User>(new_phi).get());

            // rename phi node
            std::vector<User *> should_replace;
            for (auto it = phi->uses().begin(); it != phi->uses().end(); it++) {
              auto user = (*it)->getUser();
              if (_should_not_replace.find(user) != _should_not_replace.end()) continue;
              should_replace.push_back(user);
            }

            for (const auto &user : should_replace) {
              for (auto &use : *user) {
                if (use.value() == phi) {
                  use.set(new_phi);
                }
              }
            }
            should_replace.clear();

          }
          while_end->SetOperandNum(while_end->size());


          _ssa_map.clear();
          _phi_map.clear();
          _phi_tmp_map.clear();
          _phi_origin_map.clear();
          _last_phi_map.clear();

        }

      }

    }


    A->finalize();
  }


  bool CheckLeftConstLoop(const Loop *loop, int &i, SSAPtr &end_var, SSAPtr &induction_var, SSAPtr &update_stmt) {
    auto header = loop->header();
    if (loop->blocks().size() != 3 || loop->blocks().back() != header) return false;
    auto term_inst = dyn_cast<BranchInst>(header->insts().back());
    DBG_ASSERT(term_inst != nullptr, "the last instruction of loop-condition should be branch");
    auto cond = dyn_cast<ICmpInst>(term_inst->cond());
    if (cond) {
      SSAPtr induct_var = nullptr;

      if (auto phi1 = dyn_cast<PhiNode>(cond->LHS())) {
        induct_var = cond->LHS();
        end_var = cond->RHS();
      } else if (auto phi2 = dyn_cast<PhiNode>(cond->RHS())) {
        induct_var = cond->RHS();
        end_var = cond->LHS();
      } else return false;
      if (IsSSA<ConstantInt>(end_var)) return false;

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
                  if (IsSSA<CallInst>(inst) || IsSSA<AllocaInst>(inst) || ++inst_cnt >= 30) return false;
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
      if (_in_last_loop) return phi;
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
    passinfo->Requires("NeedGcm");

    return passinfo;
  }
};

//static PassRegisterFactory<SimpleUnrollingFactory> registry;
}