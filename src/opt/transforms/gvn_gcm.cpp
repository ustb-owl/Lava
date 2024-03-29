#include "gvn_gcm.h"
#include "opt/transforms/blocksimp.h"

int GlobalValueNumbering;


namespace lava::opt {


bool GlobalValueNumberingGlobalCodeMotion::runOnFunction(const FuncPtr &F) {
  _changed = false;
  if (F->is_decl()) return _changed;

  GVN:
  PassManager::RunRequiredPasses(this);
  initialize();

  _cur_func = F.get();
  _changed = GlobalValueNumbering(F);

  // clear value number, otherwise we can't remove dead code
  _value_number.clear();

  // run dead code elimination before gcm
  auto dce = PassManager::GetTransformPass<DeadCodeElimination>("DeadCodeElimination");
  dce->initialize();
  PassManager::RunRequiredPasses(dce);
  dce->runOnFunction(F);
  dce->finalize();

#if 1
  CollectInstBlockMap(F);


  // global code motion
  auto entry = dyn_cast<BasicBlock>(F->entry()).get();
  DBG_ASSERT(_visited.empty(), "visited set is not empty");

  std::vector<InstPtr> insts;
  for (const auto &it : *F) {
    auto block = dyn_cast<BasicBlock>(it.value());
    for (const auto &inst : block->insts()) {
      auto I = dyn_cast<Instruction>(inst);
      insts.push_back(I);
    }
  }
  DBG_ASSERT(insts.size() == _user_map.size(), "instruction size is not equal _user_map");


  for (auto &inst : insts) ScheduleEarly(entry, inst);
  _visited.clear();
  for (auto &inst : insts) {
    ScheduleLate(inst);
  }

#endif

  // run block simplification
  auto blk = PassManager::GetTransformPass<BlockSimplification>("BlockSimplification");
  blk->initialize();
  _changed |= blk->runOnFunction(F);
  blk->finalize();
  if (_changed) {
    _changed = false;
    goto GVN;
  }

  return _changed;
}

void GlobalValueNumberingGlobalCodeMotion::initialize() {
  _cur_func = nullptr;
  auto func_info = PassManager::GetAnalysis<FunctionInfoPass>("FunctionInfoPass");
  _func_infos = func_info->GetFunctionInfo();
  auto dom_info = PassManager::GetAnalysis<DominanceInfo>("DominanceInfo");
  _dom_info = dom_info->GetDomInfo();
  auto loop_info = PassManager::GetAnalysis<LoopInfoPass>("LoopInfoPass");
  _loop_info = loop_info->GetLoopInfo();
}

void GlobalValueNumberingGlobalCodeMotion::finalize() {
  _value_number.clear();
  _visited.clear();
  _user_map.clear();
}

void GlobalValueNumberingGlobalCodeMotion::
Replace(const InstPtr &inst, const SSAPtr &value,
        BasicBlock *block, InstList::iterator it) {
  if (inst != value) {
    inst->ReplaceBy(value);

    auto res = std::find_if(_value_number.begin(), _value_number.end(),
                            [inst](const std::pair<SSAPtr, SSAPtr> &kv) {
                              return kv.first == inst;
                            });
    if (res != _value_number.end()) {
      _value_number.erase(res);
    }

  }
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::
FindValue(const std::shared_ptr<BinaryOperator> &binary_inst) {
  BinaryOperator::BinaryOps opcode = binary_inst->opcode();
  SSAPtr lhs = ValueOf(binary_inst->LHS());
  SSAPtr rhs = ValueOf(binary_inst->RHS());

  for (auto [k, v] : _value_number) {
    auto bin_value = dyn_cast<BinaryOperator>(k);

    if (bin_value && (bin_value != binary_inst)) {
      BinaryOperator::BinaryOps opcode2 = bin_value->opcode();
      if (opcode != opcode2) return binary_inst;
      SSAPtr lhs2 = ValueOf(bin_value->LHS());
      SSAPtr rhs2 = ValueOf(bin_value->RHS());

      bool same = false;
      if (lhs == lhs2 && rhs == rhs2) same = true;
      else if (lhs == rhs2 && rhs == lhs2) {
        if (opcode == BinaryOperator::BinaryOps::Add || opcode == BinaryOperator::BinaryOps::Mul ||
            opcode == BinaryOperator::BinaryOps::And || opcode == BinaryOperator::BinaryOps::Or) {
          same = true;
        }
      }

      if (same) return v;
    }
  }
  return binary_inst;
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::
FindValue(const std::shared_ptr<AccessInst> &access_inst) {
  for (auto [k, v] : _value_number) {
//    auto [k, v] = _value_number[i];
    auto gep = dyn_cast<AccessInst>(k);
    if (gep && (gep != access_inst)) {
      bool same = false;
      if (ValueOf(access_inst->ptr()) == ValueOf(gep->ptr())) {
        if (ValueOf(access_inst->index()) == ValueOf(gep->index())) {
          if (access_inst->size() == gep->size()) same = true;
        }
      }
      if (same) return v;
    }
  }
  return access_inst;
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::
FindValue(const std::shared_ptr<CallInst> &call_inst) {
  auto callee = dyn_cast<Function>(call_inst->Callee());
  if (!_func_infos[callee.get()].IsPure()) return call_inst;

  for (auto [k, v] : _value_number) {
//    auto [k, v] = _value_number[i];
    auto call_value = dyn_cast<CallInst>(k);
    if (call_value && (call_value->Callee() == call_inst->Callee())) {
      DBG_ASSERT(call_inst->param_size() == call_value->param_size(),
                 "parameters size of call instructions are different");
      if (call_inst->param_size() == 0) return v;
      for (auto idx = 0; idx < call_inst->param_size(); idx++) {
        if (ValueOf(call_inst->Param(idx)) != ValueOf(call_value->Param(idx))) return call_inst;
      }
      return v;
    }
  }
  return call_inst;
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::
FindValue(const std::shared_ptr<ICmpInst> &icmp_inst) {
  auto isrev = [](front::Operator a, front::Operator b) {
    return (a == front::Operator::SLess && b == front::Operator::SGreat) ||
           (a == front::Operator::SGreat && b == front::Operator::SLess) ||
           (a == front::Operator::SGreatEq && b == front::Operator::SLessEq) ||
           (a == front::Operator::SLessEq && b == front::Operator::SGreatEq);
  };

  auto op = icmp_inst->op();
  auto lhs1 = ValueOf(icmp_inst->LHS());
  auto rhs1 = ValueOf(icmp_inst->RHS());

  for (auto [k, v] : _value_number) {
//    auto [k, v] = _value_number[i];
    auto num_value = dyn_cast<ICmpInst>(k);
    if (num_value && (num_value != icmp_inst)) {
      front::Operator op2 = num_value->op();
      auto lhs2 = ValueOf(num_value->LHS());
      auto rhs2 = ValueOf(num_value->RHS());

      bool same = false;
      if (op == op2) {
        if (lhs1 == lhs2 && rhs1 == rhs2) {
          same = true;
        }
      } else if (lhs1 == rhs2 && rhs1 == lhs2 && isrev(op, op2)) {
        same = true;
      }
      if (same) return v;
    }
  }
  return icmp_inst;
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::ValueOf(const SSAPtr &value) {
  auto it = _value_number.find(value);
  if (it != _value_number.end()) return it->second;
  if (auto const_value = dyn_cast<ConstantInt>(value)) {
    // need to handle const value
    it = std::find_if(_value_number.begin(), _value_number.end(),
                      [value, &const_value](const std::pair<SSAPtr, SSAPtr> &kv) {
                        if (kv.first == value) return true;

                        auto const_kv = dyn_cast<ConstantInt>(kv.first);
                        if (const_kv && (const_value->value() == const_kv->value())) {
                          if (const_value->type()->IsIdentical(const_kv->type()))
                            return true;
                        }

                        return false;
                      });
    if (it != _value_number.end()) return it->second;
  }

  auto [res, state] = _value_number.emplace(value, value);
  DBG_ASSERT(state == true, "insert new value failed");

  // find any way
  if (auto binary_inst = dyn_cast<BinaryOperator>(value)) {
    res->second = FindValue(binary_inst);
  } else if (auto access_inst = dyn_cast<AccessInst>(value)) {
    res->second = FindValue(access_inst);
  } else if (auto call_inst = dyn_cast<CallInst>(value)) {
    res->second = FindValue(call_inst);
  }
  else if (auto icmp_inst = dyn_cast<ICmpInst>(value)) {
    res->second = FindValue(icmp_inst);
  }

  return res->second;
}

int GlobalValueNumberingGlobalCodeMotion::
GlobalValueNumbering(const FuncPtr &F) {
  bool need_loop = false;
  auto entry = dyn_cast<BasicBlock>(F->entry());
  auto rpo = _blkWalker.RPOTraverse(entry.get());

  for (const auto &BB : rpo) {
    for (auto it = BB->insts().begin(); it != BB->inst_end();) {
      auto next = std::next(it);
      if (auto binary_inst = dyn_cast<BinaryOperator>(*it)) {
        // always move const value to rhs
        if (binary_inst->LHS()->classId() == ClassId::ConstantIntId) {
          binary_inst->swapOperand();
        }

        // try to get lhs and rhs as constant value
        auto lhs_const = dyn_cast<ConstantInt>(binary_inst->LHS());
        auto rhs_const = dyn_cast<ConstantInt>(binary_inst->RHS());

        if ((lhs_const != nullptr) && (rhs_const != nullptr)) {
          auto const_inst = binary_inst->EvalArithOnConst();
          Replace(binary_inst, const_inst, BB, it);
        } else {
          need_loop = binary_inst->TryToFold();
          if (auto simp_val = binary_inst->OptimizedValue()) {
            Replace(binary_inst, simp_val, BB, it);
          } else {
            Replace(binary_inst, (need_loop ? binary_inst : ValueOf(binary_inst)), BB, it);
          }
        }
      } else if (auto call_inst = dyn_cast<CallInst>(*it)) {
        auto callee = dyn_cast<Function>(call_inst->Callee());
        if (_func_infos[callee.get()].IsPure()) {
          Replace(call_inst, ValueOf(call_inst), BB, it);
        }
      } else if (auto phi_node = dyn_cast<PhiNode>(*it)) {
        auto first = ValueOf((*phi_node)[0].value());
        bool all_same = true;
        auto size = phi_node->size();
        for (std::size_t i = 1; (i < size) && all_same; i++) {
          all_same &= (first == ValueOf((*phi_node)[i].value()));
        }
        if (all_same) Replace(phi_node, first, BB, it);
      } else if (auto access_inst = dyn_cast<AccessInst>(*it)) {
        Replace(access_inst, ValueOf(access_inst), BB, it);
      } else if (auto icmp_inst = dyn_cast<ICmpInst>(*it)) {
        // try to get lhs and rhs as constant value
        auto lhs_const = dyn_cast<ConstantInt>(icmp_inst->LHS());
        auto rhs_const = dyn_cast<ConstantInt>(icmp_inst->RHS());
        if ((lhs_const != nullptr) && (rhs_const != nullptr)) {
          auto const_inst = icmp_inst->EvalArithOnConst();
          Replace(icmp_inst, const_inst, BB, it);
        } else {
          Replace(icmp_inst, ValueOf(icmp_inst), BB, it);
        }
      }
      it = next;
    }
  }
  return need_loop;
}

void GlobalValueNumberingGlobalCodeMotion::
CollectInstBlockMap(const FuncPtr &F) {
  // clear instruction maps at first
  _user_map.clear();
  _visited.clear();

  std::size_t inst_num = 0;
  for (const auto &BB : *F) {
    auto block = dyn_cast<BasicBlock>(BB.value());
    for (const auto &inst : block->insts()) {
      auto I = dyn_cast<Instruction>(inst);
      DBG_ASSERT(I->getParent() == block.get(), "getParent is wrong");
      if (I->isBinaryOp() && I->uses().empty()) {
        ERROR("instruction is dead");
      }
      _user_map.insert({I.get(), I});
    }
    inst_num += block->insts().size();
  }
  DBG_ASSERT(_user_map.size() == inst_num, "instruction size is not equal");
}

void GlobalValueNumberingGlobalCodeMotion::
TransferInst(const InstPtr &inst, BasicBlock *new_block) {
  auto inst_block = inst->getParent();
  inst->setParent(new_block);

  if (IsSSA<BranchInst>(new_block->insts().back())) {
    auto end = --new_block->insts().end();
    new_block->insts().insert(end, inst);
  } else {
    new_block->insts().insert(--new_block->insts().end(), inst);
  }

  auto pos = inst_block->insts().begin();
  for (; pos != inst_block->insts().end(); pos++) {
    if (*pos == inst) break;
  }
  inst_block->insts().erase(pos);
}

void GlobalValueNumberingGlobalCodeMotion::
ScheduleOp(BasicBlock *entry, const InstPtr &I, const SSAPtr &operand) {
  if (auto op = dyn_cast<Instruction>(operand)) {
    ScheduleEarly(entry, op);
    auto inst_block = I->getParent();
    auto op_block = op->getParent();
    auto &dom_info = _dom_info[_cur_func];
    if (dom_info.depth[inst_block] < dom_info.depth[op_block]) {
      TransferInst(I, op_block);
    }
  }
}

void GlobalValueNumberingGlobalCodeMotion::
ScheduleEarly(BasicBlock *entry, const InstPtr &inst) {

  if (_visited.insert(inst.get()).second) {
    if (auto binary_inst = dyn_cast<BinaryOperator>(inst)) {
      TransferInst(inst, entry);
      for (auto &it : *binary_inst) {
        ScheduleOp(entry, binary_inst, it.value());
      }
    } else if (auto access_inst = dyn_cast<AccessInst>(inst)) {
      TransferInst(access_inst, entry);
      ScheduleOp(entry, access_inst, access_inst->ptr());
      ScheduleOp(entry, access_inst, access_inst->index());
    } else if (auto call_inst = dyn_cast<CallInst>(inst)) {
      if (IsPureCall(call_inst)) {
        TransferInst(inst, entry);
        for (auto &it : *call_inst) {
          ScheduleOp(entry, call_inst, it.value());
        }
      }
    } else if (auto icmp_inst = dyn_cast<ICmpInst>(inst)) {
      TransferInst(inst, entry);
      ScheduleOp(entry, icmp_inst, icmp_inst->LHS());
      ScheduleOp(entry, icmp_inst, icmp_inst->RHS());
    }
  }
}

BasicBlock *GlobalValueNumberingGlobalCodeMotion::FindLCA(BasicBlock *a, BasicBlock *b) {
  auto &info = _dom_info[_cur_func];
  BasicBlock *tmp_a = a, *tmp_b = b;
  while (info.depth[tmp_b] < info.depth[tmp_a]) tmp_a = info.idom[tmp_a];
  while (info.depth[tmp_a] < info.depth[tmp_b]) tmp_b = info.idom[tmp_b];
  while (tmp_a != tmp_b) {
    tmp_a = info.idom[tmp_a];
    tmp_b = info.idom[tmp_b];
  }
  return tmp_a;
}

void GlobalValueNumberingGlobalCodeMotion::ScheduleLate(const InstPtr &inst) {
  if (_visited.insert(inst.get()).second) {
    if (IsSSA<BinaryOperator>(inst) || IsSSA<AccessInst>(inst) || IsPureCall(inst) || IsSSA<ICmpInst>(inst)) {
      BasicBlock *lca = nullptr;

      for (const auto &use : inst->uses()) {
        auto u = use->getUser();
        DBG_ASSERT(u->isInstruction(), "user of this instruction is not an instruction");
        auto i = static_cast<Instruction *>(u);
        DBG_ASSERT(_user_map.find(i) != _user_map.end(), "can't find user cast of i");
        auto user = _user_map[i];
        ScheduleLate(user);
        auto user_block = user->getParent();

        // handle phi node
        if (auto phi_node = dyn_cast<PhiNode>(user)) {
          auto it = std::find_if(phi_node->begin(), phi_node->end(), [use](const Use &U) {
            return &U == use;
          });

          user_block = dyn_cast<BasicBlock>((*(phi_node->getParent()))[it - phi_node->begin()].value()).get();
        }
        lca = lca ? FindLCA(lca, user_block) : user_block;
      }

      DBG_ASSERT(lca != nullptr, "LCA is nullptr");
      auto best = lca;
      auto best_loop_depth = _loop_info.depth_of(best);
      while (true) {
//        TRACE("%s\n", _cur_func->GetFunctionName().c_str());
        auto cur_loop_depth = _loop_info.depth_of(lca);
        if (cur_loop_depth < best_loop_depth) {
          best = lca;
          best_loop_depth = cur_loop_depth;
        }
        if (lca == inst->getParent()) break;
        lca = _dom_info[_cur_func].idom[lca];
      }

      TransferInst(inst, best);
      for (auto it : best->insts()) {
        if (!IsSSA<PhiNode>(it)) {
          for (auto &u : inst->uses()) {
            if (u->getUser() == dyn_cast<User>(it).get()) {
              best->insts().remove(inst);
              InstList::iterator pos;
              for (pos = best->inst_begin(); pos != best->inst_end(); pos++) {
                if (*pos == it) break;
              }
              DBG_ASSERT(pos != best->insts().end(), "find its user failed");
              best->insts().insert(pos, inst);
              goto out;
            }
          }
        }
      }
      out:;
    }
  }
}

static PassRegisterFactory<GlobalValueNumberingGlobalCodeMotionFactory> registry;

}