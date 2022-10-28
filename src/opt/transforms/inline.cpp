#include <set>
#include <algorithm>

#include "opt/pass.h"
#include "lib/debug.h"
#include "common/casting.h"
#include "opt/pass_manager.h"
#include "opt/analysis/loopinfo.h"
#include "opt/analysis/funcanalysis.h"
#include "opt/transforms/blocksimp.h"

int Inlining;

namespace lava::opt {

/*
 * Function Inlining
 * 1. copy a function
 * 2. insert the new function to its caller
 */
class FunctionInlining : public FunctionPass {
public:
  using RetInstPtr = std::shared_ptr<ReturnInst>;
  using CallInstPtr = std::shared_ptr<CallInst>;

private:
  Module *module;
  FuncPtr callee;
  LoopInfo loop_info;
  FuncInfoMap func_infos;
  RetInstPtr ret_inst;
  std::unordered_map<SSAPtr, SSAPtr> ssa_map;
  std::unordered_map<BlockPtr, BlockPtr> blk_map;
  std::vector<std::shared_ptr<PhiNode>> phi_list;
  std::stack<BlockPtr> pos_stack;

  const std::size_t inst_cnt_threshold = 50;
  const std::size_t blk_cnt_threshold = 10;
  const std::string inline_entry_name = "inline_entry";
  const std::string inline_exit_name = "inline_exit";
  static std::size_t inline_entry_id;
  static std::size_t inline_exit_id;

public:
  bool runOnFunction(const FuncPtr &F) final {
    bool changed = false;

    // just return if this function is a leaf
    auto &func_info = func_infos[F.get()];
    if (func_info.is_leaf) return changed;

    module = F->getParent();
    // get loop info
    auto loop_analysis = PassManager::GetAnalysis<LoopInfoPass>("LoopInfoPass");
    loop_analysis->initialize();
    loop_analysis->runOnFunction(F);
    loop_info = loop_analysis->GetLoopInfo();

    // collect call instructions
    std::vector<CallInstPtr> call_insts;
    for (const auto &BB : F->GetBlockList()) {
      for (const auto &inst : BB->insts()) {
        if (auto call_inst = dyn_cast<CallInst>(inst)) {
          call_insts.push_back(call_inst);
        }
      }
    }

    bool become_leaf = true;
    for (auto &call_inst : call_insts) {
      if (Inlinable(F, call_inst)) {
        changed = true;

        // perform inlining
        auto candidate = cast<Function>(call_inst->Callee());
        CopyFunction(candidate);
//        callee->dump();
//        candidate->dump();
//        F->dump();
        ConnectUD(call_inst);
//        F->dump();

        // update function info
        auto callee_info = func_infos[candidate.get()];
        func_info.load_global |= callee_info.load_global;
        func_info.store_global |= callee_info.store_global;
        func_info.load_global_array |= callee_info.load_global_array;
        func_info.has_size_effect |= callee_info.has_size_effect;
        Clear();
      } else {
        become_leaf = false;
      }
    }

    // update function info
    if (!func_info.is_leaf)
      func_info.is_leaf = become_leaf;

    // run block simplification
    auto blk = PassManager::GetTransformPass<BlockSimplification>("BlockSimplification");
    blk->initialize();
    changed |= blk->runOnFunction(F);
    blk->finalize();

    return changed;
  }

  void initialize() final {
    ret_inst = nullptr;
    auto func_info = PassManager::GetAnalysis<FunctionInfoPass>("FunctionInfoPass");
    func_infos = func_info->GetFunctionInfo();
  }

  void finalize() final {
    Clear();
    func_infos.clear();
  }

  void Clear() {
    ssa_map.clear();
    blk_map.clear();
    phi_list.clear();
  }

  // check if this function is inlinable
  bool Inlinable(const FuncPtr &F, const CallInstPtr &call) const;

  // get the total instruction count of this function
  std::size_t GetInstCount(const FuncPtr &F) const;

  // rename phi nodes
  void Rename();

  // copy the basic block
  void CopyBasicBlock(const BlockPtr &BB);

  // copy values
  SSAPtr CopyValue(const SSAPtr &value);

  // Get copied operand
  SSAPtr GetOperand(const SSAPtr &value);

  // copy instructions
  void CopyInstruction(const InstPtr &inst);

  // copy the callee function
  void CopyFunction(const FuncPtr &F);

  // connect ud chain
  void ConnectUD(const CallInstPtr &call_inst);
};

std::size_t FunctionInlining::inline_entry_id = 0;
std::size_t FunctionInlining::inline_exit_id = 0;


std::size_t FunctionInlining::GetInstCount(const FuncPtr &F) const {
  std::size_t cnt = 0;
  for (const auto &it : *F) {
    auto blk = dyn_cast<BasicBlock>(it.value());
    cnt += blk->insts().size();
  }
  return cnt;
}

void FunctionInlining::Rename() {
  for (const auto &orig_phi : phi_list) {
    auto phi = dyn_cast<PhiNode>(ssa_map[orig_phi]);
    phi->SetOperandNum(phi->getParent()->size());
    phi->Reserve();

    auto orig_preds = orig_phi->blocks();
    auto preds = phi->blocks();
    uint32_t idx = 0;
    for (const auto &it : *orig_phi) {
      auto pred = orig_preds[idx];
      std::size_t i;
      for (i = 0; i < orig_preds.size(); i++) {
        if (blk_map[pred] == preds[i])
          break;
      }
      phi->SetOperand(i, GetOperand(it.value()));
      idx++;
    }
    DBG_ASSERT(phi->size() == orig_phi->size(), "The operand number of PHI instruction is wrong");
  }
}

bool FunctionInlining::Inlinable(const FuncPtr &F, const std::shared_ptr<CallInst> &call) const {
  auto candidate = cast<Function>(call->Callee());
  if (candidate->is_decl())
    return false;
  if (GetInstCount(candidate) > inst_cnt_threshold)
    return false;
  if (loop_info.depth_of(call->getParent()) != 0)
    return false;
  if (F == candidate)
    return false;
  if (candidate->size() > blk_cnt_threshold)
    return false;
  return true;
}

SSAPtr FunctionInlining::CopyValue(const SSAPtr &value) {
  if (IsSSA<GlobalVariable>(value)) {
    return value;
  } else if (value->IsConst()) {
    return std::static_pointer_cast<ConstantValue>(value)->Copy();
  } else if (IsSSA<UnDefineValue>(value)) {
    return std::make_shared<UnDefineValue>();
  } else {
    ERROR("Unknown value");
  }
}

SSAPtr FunctionInlining::GetOperand(const SSAPtr &value) {
  if (value->isInstruction()) {
    if (!ssa_map.count(value)) {
      CopyInstruction(cast<Instruction>(value));
    }
    DBG_ASSERT(ssa_map.count(value), "This value hasn't been not copied");
    return ssa_map[value];
  } else if (value->isBlock()) {
    return blk_map[cast<BasicBlock>(value)];
  } else if (value->IsArgument()) {
    return ssa_map[value];
  } else {
    return CopyValue(value);
  }
}

void FunctionInlining::CopyInstruction(const InstPtr &inst) {
  SSAPtr copied_inst = nullptr;

  // preserve the insert point
  pos_stack.push(module->InsertPoint());

  // update the insert pointer
  BlockPtr insert_point = nullptr;
  for (const auto &[k, v] : blk_map) {
    if (k.get() == inst->getParent()) insert_point = v;
  }
  module->SetInsertPoint(insert_point);

  switch (inst->classId()) {
    case ClassId::ReturnInstId: {
      SSAPtr ret_val = nullptr;
      if (auto val = cast<ReturnInst>(inst)->RetVal())
        ret_val = GetOperand(val);
      copied_inst = module->CreateReturn(ret_val);
      ret_inst = cast<ReturnInst>(copied_inst);
      break;
    }
    case ClassId::BranchInstId: {
      auto orig_branch = cast<BranchInst>(inst);
      auto cond = GetOperand(orig_branch->cond());
      auto true_block = GetOperand(orig_branch->true_block());
      auto false_block = GetOperand(orig_branch->false_block());

      // update predecessor
      cast<BasicBlock>(true_block)->AddValue(insert_point);
      cast<BasicBlock>(false_block)->AddValue(insert_point);

      copied_inst = module->AddInst<BranchInst>(cond, true_block, false_block);
      copied_inst->set_type(nullptr);
      break;
    }
    case ClassId::JumpInstId: {
      auto orig_jump = cast<JumpInst>(inst);
      auto target = GetOperand(orig_jump->target());
      copied_inst = module->CreateJump(cast<BasicBlock>(target));
      break;
    }
    case ClassId::BinaryOperatorId: {
      auto orig_bin = cast<BinaryOperator>(inst);
      auto lhs = GetOperand(orig_bin->LHS());
      auto rhs = GetOperand(orig_bin->RHS());
      copied_inst = BinaryOperator::Create(BinaryOperator::BinaryOps(orig_bin->opcode()), lhs, rhs);
      copied_inst->set_type(orig_bin->type());

      // add inst into basic block
      auto bin = cast<Instruction>(copied_inst);
      insert_point->AddInstToEnd(bin);
      bin->setParent(insert_point.get());
      copied_inst = bin;
      break;
    }
    case ClassId::AllocaInstId: {
      copied_inst = module->AddInst<AllocaInst>();
      copied_inst->set_type(inst->type());
      break;
    }
    case ClassId::LoadInstId: {
      auto orig_load = cast<LoadInst>(inst);
      auto ptr = GetOperand(orig_load->Pointer());
      copied_inst = module->CreateLoad(ptr);
      break;
    }
    case ClassId::StoreInstId: {
      auto orig_store = cast<StoreInst>(inst);
      auto val = GetOperand(orig_store->data());
      auto ptr = GetOperand(orig_store->pointer());
      copied_inst = module->AddInst<StoreInst>(val, ptr);
      copied_inst->set_type(nullptr);
      break;
    }
    case ClassId::AccessInstId: {
      auto orig_access = cast<AccessInst>(inst);
      auto ptr = GetOperand(orig_access->ptr());
      SSAPtrList index;
      for (const auto &it : *orig_access) {
        index.push_back(GetOperand(it.value()));
      }
      index.pop_front();

      auto acc_type = AccessInst::AccessType::Element;
      copied_inst = module->AddInst<AccessInst>(acc_type, ptr, index);
      copied_inst->set_type(inst->type());
      break;
    }
    case ClassId::CastInstId: {
      auto orig_cast = cast<CastInst>(inst);
      auto value = GetOperand(orig_cast->operand());
      copied_inst = module->AddInst<CastInst>(CastInst::CastOps(orig_cast->opcode()), value);
      copied_inst->set_type(inst->type());
      break;
    }
    case ClassId::ICmpInstId: {
      auto orig_icmp = cast<ICmpInst>(inst);
      auto lhs = GetOperand(orig_icmp->LHS());
      auto rhs = GetOperand(orig_icmp->RHS());
      copied_inst = module->AddInst<ICmpInst>(orig_icmp->op(), lhs, rhs);
      copied_inst->set_type(inst->type());
      break;
    }
    case ClassId::PHINodeId: {
      auto orig_phi = cast<PhiNode>(inst);
      auto bb = module->InsertPoint();
      auto phi = std::make_shared<PhiNode>(bb.get());

      // insert this phi node to the head of dom_frontier
      auto dom_frontier = bb;
      auto begin = dom_frontier->insts().begin();
      dom_frontier->insts().insert(begin, phi);

      phi_list.push_back(orig_phi);
      phi->set_type(inst->type());

      // rename in the later phase
      copied_inst = phi;
      break;
    }
    case ClassId::CallInstId: {
      std::vector<SSAPtr> args;
      auto orig_call = cast<CallInst>(inst);
      auto new_callee = orig_call->Callee();
      for (int i = 0; i < orig_call->param_size(); i++) {
        args.push_back(GetOperand(orig_call->Param(i)));
      }
      copied_inst = module->AddInst<CallInst>(new_callee, args);
      copied_inst->set_type(inst->type());
      break;
    }
    default:
      DBG_ASSERT(false, "This SSA can't be copied");
  }

  // recover the old insert point
  module->SetInsertPoint(pos_stack.top());
  pos_stack.pop();

  DBG_ASSERT(copied_inst != nullptr, "copy the instruction failed");
  ssa_map.emplace(inst, copied_inst);
}

void FunctionInlining::CopyBasicBlock(const BlockPtr &BB) {
  module->SetInsertPoint(blk_map[BB]);
  for (const auto &inst : BB->insts()) {
    CopyInstruction(inst);
  }
}

void FunctionInlining::CopyFunction(const FuncPtr &F) {

  // create a new copy of the callee
  std::string callee_name = F->GetFunctionName() + "_cpy";
  callee = module->CreateFunction(callee_name, F->type());
  callee->set_logger(F->logger());

  // copy arguments
  for (const auto &it : F->args()) {
    auto arg = cast<ArgRefSSA>(it);
    auto new_arg = module->CreateArgRef(callee, arg->index(), arg->arg_name());
    ssa_map[arg] = new_arg;
  }

  // copy blocks
  auto blocks = F->GetBlockList();

  // set function entry
  module->SetFuncEntry(blocks[0]);

  // create all blocks first
  for (const auto &bb : blocks) {
    auto new_bb = module->CreateBlock(callee, bb->name());
    new_bb->SetOperandNum(bb->operandNum());
    blk_map[bb] = new_bb;
  }
  DBG_ASSERT(blk_map.size() == blocks.size(), "This number of basic blocks is wrong");

  for (const auto &bb : blocks) {
//    bb->dump();
    CopyBasicBlock(bb);
  }

  // rename phi nodes
  Rename();
}

void FunctionInlining::ConnectUD(const CallInstPtr &call_inst) {
  auto BB = call_inst->getParent();
  auto F = BB->getParent();
  auto &insts = BB->insts();
  auto orig_succ = BB->successors();

  // insert blocks of callee
  // copy blocks
  auto blocks = callee->GetBlockList();

  // replace the blocks' user with F
  for (auto &use : *callee) {
    auto val = use.value();
    F->AddValue(val);
    cast<BasicBlock>(val)->setParent(F);
  }

  // handle the argument
  if (call_inst->param_size() != 0) {
    auto &args = callee->args();
    for (int i = 0; i < call_inst->param_size(); i++) {
      auto param = call_inst->Param(i);
      auto formal_arg = args[i];
      formal_arg->ReplaceBy(param);
    }
    // clear all arguments
    args.clear();
  }

  auto func_type = cast<FuncType>(callee->type());
  if (!func_type->GetReturnType(func_type->GetArgsType().value())->IsVoid()) {
    // handle the return value
    DBG_ASSERT(ret_inst != nullptr, "The ReturnInst is nullptr");
    auto ret_val = ret_inst->RetVal();
    call_inst->ReplaceBy(ret_val);
  }

  // create a jump to the entry block
  auto call_pos = call_inst->GetPosition();
  auto callee_entry = cast<BasicBlock>(callee->entry());
  auto callee_shared = std::find_if(F->begin(), F->end(), [BB](const Use &use) {
    return use.value().get() == BB;
  });

  callee_entry->SetBlockName(inline_entry_name + std::to_string(inline_entry_id));
  auto jump_inst = std::make_shared<JumpInst>(callee_entry);
  jump_inst->setParent(BB);
  insts.insert(call_pos, jump_inst);
  callee_entry->AddValue(callee_shared->value());
  call_inst->Clear();
  auto pos = call_inst->RemoveFromParent();

  // move the rest instructions to the exit block of the callee
  auto exit = ret_inst->getParent();
  exit->SetBlockName(inline_exit_name + std::to_string(inline_exit_id));
  auto exit_shared = std::find_if(blocks.begin(), blocks.end(), [exit](const BlockPtr &bb) {
    return bb.get() == exit;
  });
  DBG_ASSERT(exit_shared != blocks.end(), "Can't find the exit block");
  auto &exit_insts = exit->insts();

  // update parents
  for (auto it = pos; it != insts.end(); it++) {
    it->get()->setParent(exit);
  }
  exit_insts.insert(exit->inst_end(), pos, insts.end());
  insts.erase(pos, insts.end());

  // update the successor's use list
  for (auto &it : orig_succ) {
    for (std::size_t i = 0; i < it->size(); i++) {
      auto pred = it->GetOperand(i);
      if (pred.get() == BB) {
        it->AddValue(*exit_shared);
        std::swap((*it)[i], it->back());
        it->RemoveValue(it->size() - 1);
        break;
      }
    }
  }

  // remove the CallInst and ReturnInst
  ret_inst->Clear();
  ret_inst->RemoveFromParent();

  for (auto &use : *callee) {
    use.set(nullptr);
  }
  callee->RemoveValue(nullptr);

  // remove the copied function from module
//  callee->RemoveFromParent();
    callee->is_copied(true);

  // update id
  inline_entry_id++;
  inline_exit_id++;
}

class FunctionInliningFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<FunctionInlining>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "FunctionInlining", false, 2, FUNCTION_INLINING);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};

static PassRegisterFactory<FunctionInliningFactory> registry1;

class FunctionCleanUp : public ModulePass {
public:
  bool runOnModule(Module &M) final {
    auto &functions = M.Functions();
    auto res = functions.erase(std::remove_if(functions.begin(), functions.end(), [](const FuncPtr &F) {
      return F->is_copied();
    }), functions.end());
    return res != functions.end();
  }
};

class FunctionCleanUpFactory : public PassFactory {
public:
  PassInfoPtr CreatePass(PassManager *) override {
    auto pass = std::make_shared<FunctionCleanUp>();
    auto passinfo =
        std::make_shared<PassInfo>(pass, "FunctionCleanUp", false, 2, FUNCTION_CLEANUP);
    passinfo->Requires("FunctionInfoPass");
    return passinfo;
  }
};

static PassRegisterFactory<FunctionCleanUpFactory> registry2;


}