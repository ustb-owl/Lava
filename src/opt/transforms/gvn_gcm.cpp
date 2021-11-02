#include "gvn_gcm.h"
#include "opt/transforms/blocksimp.h"

int GlobalValueNumbering;

#define OPEN 0

namespace lava::opt {


bool GlobalValueNumberingGlobalCodeMotion::runOnFunction(const FuncPtr &F) {
  _changed = false;
  if (F->is_decl()) return _changed;
  auto A = PassManager::GetAnalysis<NeedGcm>("NeedGcm");
  if (A->IsCrypto()) return _changed;
  TRACE0();

  _cur_func = F.get();

  GVN:
  GlobalValueNumbering(F);

  // clear value number, otherwise we can't remove dead code
  _value_number.clear();

  // run dead code elimination before gcm
  auto dce = PassManager::GetTransformPass<DeadCodeElimination>("DeadCodeElimination");
  dce->initialize();
  dce->runOnFunction(F);
  dce->finalize();


#if 1
  if (!A->IsNeedGcm()) return _changed;
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
  for (auto &inst : insts) ScheduleLate(inst);
#endif

  // run block simplification
  auto blk = PassManager::GetTransformPass<BlockSimplification>("BlockSimplification");
  blk->initialize();
  auto changed = false;
//  changed = blk->runOnFunction(F);
  blk->finalize();
  if (changed) goto GVN;

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
  _inst_block_map.clear();
}

void GlobalValueNumberingGlobalCodeMotion::
Replace(const InstPtr &inst, const SSAPtr &value,
        BasicBlock *block, SSAPtrList::iterator it) {
  if (inst != value) {
    inst->ReplaceBy(value);

    auto res = std::find_if(_value_number.begin(), _value_number.end(),
                            [inst](const std::pair<SSAPtr, SSAPtr> &kv) {
                              return kv.first == inst;
                            });
    if (res != _value_number.end()) {
      std::swap(*res, _value_number.back());
      _value_number.pop_back();
    }

  }
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::
FindValue(const std::shared_ptr<BinaryOperator> &binary_inst) {
  BinaryOperator::BinaryOps opcode = binary_inst->opcode();
  SSAPtr lhs = ValueOf(binary_inst->LHS());
  SSAPtr rhs = ValueOf(binary_inst->RHS());

  for (std::size_t i = 0; i < _value_number.size(); i++) {
    auto[k, v] = _value_number[i];

    auto bin_value = dyn_cast<BinaryOperator>(binary_inst);


    if (bin_value && (bin_value != binary_inst)) {
      BinaryOperator::BinaryOps opcode2 = bin_value->opcode();
      SSAPtr lhs2 = ValueOf(bin_value->LHS());
      SSAPtr rhs2 = ValueOf(bin_value->RHS());

      bool same = false;
      if (opcode == opcode2) {
        if (lhs == lhs2 && rhs == rhs2) same = true;
        else if (lhs == rhs2 && rhs == lhs2) {
          if (opcode == BinaryOperator::BinaryOps::Add || opcode == BinaryOperator::BinaryOps::Mul ||
              opcode == BinaryOperator::BinaryOps::And || opcode == BinaryOperator::BinaryOps::Or) {
            same = true;
          }
        }
      }
      if (same) return v;
    }
  }
  return binary_inst;
}

SSAPtr GlobalValueNumberingGlobalCodeMotion::
FindValue(const std::shared_ptr<AccessInst> &access_inst) {
  for (std::size_t i = 0; i < _value_number.size(); i++) {
    auto[k, v] = _value_number[i];
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

  for (std::size_t i = 0; i < _value_number.size(); i++) {
    auto[k, v] = _value_number[i];
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

  for (std::size_t i = 0; i < _value_number.size(); i++) {
    auto[k, v] = _value_number[i];
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
  auto it = std::find_if(_value_number.begin(), _value_number.end(), [value](const std::pair<SSAPtr, SSAPtr> &kv) {
    return kv.first == value;
  });
  if (it != _value_number.end()) return it->second;

  uint32_t idx = _value_number.size();
  _value_number.emplace_back(value, value);

  // find any way
  if (auto binary_inst = dyn_cast<BinaryOperator>(value)) {
    _value_number[idx].second = FindValue(binary_inst);
  } else if (auto access_inst = dyn_cast<AccessInst>(value)) {
    _value_number[idx].second = FindValue(access_inst);
  } else if (auto call_inst = dyn_cast<CallInst>(value)) {
    _value_number[idx].second = FindValue(call_inst);
  }
#if OPEN
  else if (auto icmp_inst = dyn_cast<ICmpInst>(value)) {
    _value_number[idx].second = FindValue(icmp_inst);
  }
#endif

  return _value_number[idx].second;
}

void GlobalValueNumberingGlobalCodeMotion::
GlobalValueNumbering(const FuncPtr &F) {
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
          binary_inst->TryToFold();
          if (auto simp_val = binary_inst->OptimizedValue()) {
            Replace(binary_inst, simp_val, BB, it);
          } else {
            Replace(binary_inst, ValueOf(binary_inst), BB, it);
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
          all_same = first == ValueOf((*phi_node)[i].value());
        }
        if (all_same) Replace(phi_node, first, BB, it);
      } else if (auto icmp_inst = dyn_cast<ICmpInst>(*it)) {
#if OPEN
        // try to get lhs and rhs as constant value
        auto lhs_const = dyn_cast<ConstantInt>(icmp_inst->LHS());
        auto rhs_const = dyn_cast<ConstantInt>(icmp_inst->RHS());
        if ((lhs_const != nullptr) && (rhs_const != nullptr)) {
          auto const_inst = icmp_inst->EvalArithOnConst();
          Replace(icmp_inst, const_inst, BB, it);
        } else {
          Replace(icmp_inst, ValueOf(icmp_inst), BB, it);
        }
#endif
      }

      it = next;
    }
  }
}

void GlobalValueNumberingGlobalCodeMotion::
CollectInstBlockMap(const FuncPtr &F) {
  std::size_t inst_num = 0;
  for (const auto &BB : *F) {
    auto block = dyn_cast<BasicBlock>(BB.value());
    for (const auto &inst : block->insts()) {
      auto I = dyn_cast<Instruction>(inst);
      if (I->isBinaryOp() && I->uses().empty()) {
        ERROR("instruction is dead");
      }
      _inst_block_map.insert({I.get(), block.get()});
      _user_map.insert({I.get(), I});
    }
    inst_num += block->insts().size();
  }
  DBG_ASSERT(_user_map.size() == inst_num, "instruction size is not equal");
}

void GlobalValueNumberingGlobalCodeMotion::
TransferInst(const InstPtr &inst, BasicBlock *new_block) {
  DBG_ASSERT(_inst_block_map.find(inst.get()) != _inst_block_map.end(), "can't find this inst's block");
  auto inst_block = _inst_block_map[inst.get()];
  _inst_block_map[inst.get()] = new_block;
  new_block->insts().insert(--new_block->insts().end(), inst);

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
    DBG_ASSERT(_inst_block_map.find(I.get()) != _inst_block_map.end(), "can't find this inst's block");
    auto inst_block = _inst_block_map[I.get()];
    DBG_ASSERT(_inst_block_map.find(op.get()) != _inst_block_map.end(), "can't find this op's block");
    auto op_block = _inst_block_map[op.get()];
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
    }
//      else if (auto access_inst = dyn_cast<AccessInst>(inst)) {
//        TransferInst(access_inst, entry);
//        ScheduleOp(entry, access_inst, access_inst->ptr());
//        ScheduleOp(entry, access_inst, access_inst->index());
//      }
  }
}

BasicBlock *GlobalValueNumberingGlobalCodeMotion::FindLCA(BasicBlock *a, BasicBlock *b) {
  auto &info = _dom_info[_cur_func];
  while (info.depth[b] < info.depth[a]) a = info.idom[a];
  while (info.depth[a] < info.depth[b]) b = info.idom[b];
  while (a != b) {
    a = info.idom[a];
    b = info.idom[b];
  }
  return a;
}

void GlobalValueNumberingGlobalCodeMotion::ScheduleLate(const InstPtr &inst) {
  if (_visited.insert(inst.get()).second) {
    if (IsSSA<BinaryOperator>(inst)) {
      BasicBlock *lca = nullptr;

      for (const auto &use : inst->uses()) {
        auto u = use->getUser();
        DBG_ASSERT(u->isInstruction(), "user of this instruction is not an instruction");
        auto i = static_cast<Instruction *>(u);
        DBG_ASSERT(_user_map.find(i) != _user_map.end(), "can't find user cast of i");
        auto user = _user_map[i];
        ScheduleLate(user);
        auto user_block = _inst_block_map[user.get()];

        // handle phi node
        if (auto phi_node = dyn_cast<PhiNode>(user)) {
          auto it = std::find_if(phi_node->begin(), phi_node->end(), [use](const Use &U) {
            return &U == use;
          });

          user_block = dyn_cast<BasicBlock>((*(phi_node->parent_block()))[it - phi_node->begin()].value()).get();
        }
        lca = lca ? FindLCA(lca, user_block) : user_block;
      }

      DBG_ASSERT(lca != nullptr, "LCA is nullptr");
      auto best = lca;
      auto best_loop_depth = _loop_info.depth_of(best);
      while (true) {
        auto cur_loop_depth = _loop_info.depth_of(lca);
        if (cur_loop_depth < best_loop_depth) {
          best = lca;
          best_loop_depth = cur_loop_depth;
        }
        if (lca == _inst_block_map[inst.get()]) break;
        lca = _dom_info[_cur_func].idom[lca];
      }

      TransferInst(inst, best);
      for (auto it : best->insts()) {
        if (!IsSSA<PhiNode>(it)) {
          for (auto &u : inst->uses()) {
            if (u->getUser() == dyn_cast<User>(it).get()) {
              best->insts().remove(inst);
              SSAPtrList::iterator pos;
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