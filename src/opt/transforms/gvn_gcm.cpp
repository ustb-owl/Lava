#include <algorithm>

#include "opt/pass.h"
#include "opt/blkwalker.h"
#include "common/casting.h"
#include "opt/pass_manager.h"
#include "opt/analysis/funcanalysis.h"

int GlobalValueNumbering;

namespace lava::opt {

using ValueNumber = std::vector<std::pair<SSAPtr, SSAPtr>>;

/*
 * Global value numbering and global code motion
 * TODO: swapOperand or FoldBinary would cause int_literal WA
 */
class GlobalValueNumberingGlobalCodeMotion : public FunctionPass {
private:
  bool               _changed;
  BlockWalker        _blkWalker;
  ValueNumber        _value_number;
  FuncInfoMap        _func_infos;

public:

  bool runOnFunction(const FuncPtr &F) final {
    _changed = false;
    if (F->is_decl()) return _changed;

    GlobalValueNumbering(F);

    return _changed;
  }

  void initialize() final {
    auto A = PassManager::GetAnalysis<FunctionInfoPass>("FunctionInfoPass");
    _func_infos = A->GetFunctionInfo();
  }

  void finalize() final {
    _value_number.clear();
  }

  void Replace(const InstPtr &inst, const SSAPtr &value, BasicBlock *block, SSAPtrList::iterator it) {
    if (inst != value) {
      inst->ReplaceBy(value);

      auto res = std::find_if(_value_number.begin(), _value_number.end(),
          [inst](const std::pair<SSAPtr , SSAPtr> &kv) {
            return kv.first == inst;
      });
      if (res != _value_number.end()) {
        std::swap(*res, _value_number.back());
        _value_number.pop_back();
      }

      DBG_ASSERT(*it == inst, "iterator is not current instruction");
      block->insts().erase(it);
    }
  }

  SSAPtr FindValue(const std::shared_ptr<BinaryOperator> &binary_inst) {
    BinaryOperator::BinaryOps opcode = binary_inst->opcode();
    SSAPtr lhs = ValueOf(binary_inst->LHS());
    SSAPtr rhs = ValueOf(binary_inst->RHS());

    for (auto i = 0; i < _value_number.size(); i++) {
      auto [k, v] = _value_number[i];

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

  SSAPtr FindValue(const std::shared_ptr<AccessInst> &access_inst) {
    for (auto i = 0; i < _value_number.size(); i++) {
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

  SSAPtr FindValue(const std::shared_ptr<CallInst> &call_inst) {
    auto callee = dyn_cast<Function>(call_inst->Callee());
    if (!_func_infos[callee.get()].IsPure()) return call_inst;

    for (auto i = 0; i < _value_number.size(); i++) {
      auto [k, v] = _value_number[i];
      auto call_value = dyn_cast<CallInst>(k);
      if (call_value && (call_value->Callee() == call_inst->Callee())) {
        DBG_ASSERT(call_inst->param_size() == call_value->param_size(), "parameters size of call instructions are different");
        if (call_inst->param_size() == 0) return v;
        for (auto idx = 0; idx < call_inst->param_size(); idx++) {
          if (ValueOf(call_inst->Param(idx)) != ValueOf(call_value->Param(idx))) return call_inst;
        }
        return v;
      }
    }
    return call_inst;
  }

  SSAPtr ValueOf(const SSAPtr &value) {
    auto it = std::find_if(_value_number.begin(), _value_number.end(), [value](const std::pair<SSAPtr , SSAPtr> &kv) {
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

    return _value_number[idx].second;
  }

  void GlobalValueNumbering(const FuncPtr &F) {
    auto entry = dyn_cast<BasicBlock>(F->entry());
    auto rpo = _blkWalker.RPOTraverse(entry.get());

    for (const auto &BB : rpo) {
      for (auto it = BB->insts().begin(); it != BB->inst_end(); ) {
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
          for (auto i = 1; (i < size) && all_same; i++) {
            all_same = first == ValueOf((*phi_node)[i].value());
          }
          if (all_same) Replace(phi_node, first, BB, it);
        }

        it = next;
      }
    }


  }

};

class GlobalValueNumberingGlobalCodeMotionFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<GlobalValueNumberingGlobalCodeMotion>();
    auto passinfo = std::make_shared<PassInfo>(pass, "GlobalValueNumberingGlobalCodeMotion", false, 1, GVN_GCM);
    passinfo->Requires("FunctionInfoPass");
//    passinfo->Requires("LoopInfoPass");
    return passinfo;
  }
};

static PassRegisterFactory<GlobalValueNumberingGlobalCodeMotionFactory> registry;

}