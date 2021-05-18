#include "irbuilder.h"

namespace lava::mid {

SSAPtr IRBuilder::visit(IntAST *node) {
  return _module.CreateConstInt(node->value());
}

SSAPtr IRBuilder::visit(CharAST *node) {
  return nullptr;
}

SSAPtr IRBuilder::visit(StringAST *node) {
  return nullptr;
}

SSAPtr IRBuilder::visit(VariableAST *node) {
  auto var_ssa = _module.GetValues(node->id());
  DBG_ASSERT(var_ssa != nullptr, "variable not found");
  return var_ssa;
}

SSAPtr IRBuilder::visit(VariableDecl *node) {
  auto context = _module.SetContext(node->logger());

  // save current insert point
  auto cur_insert = _module.InsertPoint();

  // update insert at entry
  for (const auto &it : node->defs()) {
    it->CodeGeneAction(this);
  }
  _module.SetInsertPoint(cur_insert);
  return nullptr;
}

SSAPtr IRBuilder::visit(VariableDefAST *node) {
  auto variable = _module.CreateAlloca(node->ast_type());

  auto symtab = _module.ValueSymTab();
  symtab->AddItem(node->id(), variable);

  if (node->hasInit()) {
    auto init_ssa = node->init()->CodeGeneAction(this);
    DBG_ASSERT(init_ssa != nullptr, "emit init value failed");

    auto last_pos = _module.FuncEntry()->insts().end();
    _module.SetInsertPoint(_module.FuncEntry(), --last_pos);
    _module.CreateAssign(variable, init_ssa);
  }

  return nullptr;
}

SSAPtr IRBuilder::visit(BinaryStmt *node) {
  auto lhs = node->lhs()->CodeGeneAction(this);
  DBG_ASSERT(lhs != nullptr, "lhs generate failed");
  auto rhs = node->rhs()->CodeGeneAction(this);
  DBG_ASSERT(rhs != nullptr, "rhs generate failed");
  auto bin_inst = _module.CreateBinaryOperator(node->op(), lhs, rhs);
  DBG_ASSERT(bin_inst != nullptr, "binary statement generate failed");

  return bin_inst;
}

SSAPtr IRBuilder::visit(UnaryStmt *node) {
  return nullptr;
}

// TODO:: emit control stmts
SSAPtr IRBuilder::visit(ControlAST *node) {
#if 0
  auto context = _module.SetContext(node->logger());
  if (!node->hasReturnVal()) return nullptr;

  // create a new block
  const auto &func = _module.InsertPoint()->parent();
  auto new_block = _module.CreateBlock(func);

  // generate return value
  auto retval = node->getReturn()->CodeGeneAction(this);
  DBG_ASSERT(retval != nullptr, "emit return value failed");

  // copy return value
  auto store_inst = _module.CreateAssign(_module.ReturnValue(), retval);
  DBG_ASSERT(store_inst != nullptr, "copy return value failed");

  auto jump_inst = _module.CreateJump(_module.FuncExit());
  DBG_ASSERT(jump_inst != nullptr, "emit jump instruction failed");

  // set insert point at new block
  _module.SetInsertPoint(new_block);
#endif
  return nullptr;
}

SSAPtr IRBuilder::visit(CompoundStmt *node) {
  // make new environment when not in function
  auto guard = !_in_func ? NewEnv() : xstl::Guard(nullptr);
  bool set_name = false;
  if (_in_func) {
    set_name = true;
    _in_func = false;
  }

  // create new block
  const auto &cur_func = _module.InsertPoint()->parent();
  auto block = _module.CreateBlock(cur_func, (set_name ? "body" : "block"));
  _module.CreateJump(block);
  _module.SetInsertPoint(block);
  for (const auto &it : node->stmts()) {
    it->CodeGeneAction(this);
  }
  return block;
}

// create if-then-else blocks
/*
 *            cond_block
 *           /          \
 *         then        else
 *          |            |  ------> else and else_ssa and be merged in the opt stage
 *       then_ssa    else_ssa
 *           \          /
 *            end_block
 */
SSAPtr IRBuilder::visit(IfElseStmt *node) {
  auto context = _module.SetContext(node->logger());
  auto func = _module.InsertPoint()->parent();

  // create condition block
  auto &cond = node->cond();
  auto cond_block = cond->CodeGeneAction(this);
  auto then_block = _module.CreateBlock(func, "if.then");
  auto else_block = _module.CreateBlock(func, "if.else");

  auto cond_ssa = cond->CodeGeneAction(this);
  DBG_ASSERT(cond_ssa != nullptr, "emit condition statement failed");
  _module.CreateBranch(cond_ssa, then_block, else_block);

  // create if end block
  auto end_block = _module.CreateBlock(func, "if.end");

  // create then block
  auto &then_ast = node->then();
  _module.SetInsertPoint(then_block);
  auto then_ssa = then_ast->CodeGeneAction(this);
  DBG_ASSERT(then_ssa != nullptr, "emit then block failed");
  _module.CreateJump(end_block);

  // create else block
  _module.SetInsertPoint(else_block);
  if (node->hasElse()) {
    auto &else_ast = node->else_then();
    auto else_ssa = else_ast->CodeGeneAction(this);
    DBG_ASSERT(else_ssa != nullptr, "emit else block failed");
  }
  _module.CreateJump(end_block);

  // set end_block as the insert point
  _module.SetInsertPoint(end_block);

  return nullptr;
}

// TODO: fix here
SSAPtr IRBuilder::visit(CallStmt *node) {
  auto context = _module.SetContext(node->logger());

  // TODO: callee not sure
  auto callee = node->expr()->CodeGeneAction(this);
  DBG_ASSERT(callee != nullptr, "can't generate callee here");

  // emit args
  std::vector<SSAPtr> args;
  for (const auto &it : node->args()) {
    auto arg = it->CodeGeneAction(this);
    DBG_ASSERT(arg != nullptr, "emit arg failed");
    args.push_back(arg);
  }

  auto call_inst = _module.CreateCallInst(callee, args);

  return call_inst;
}

SSAPtr IRBuilder::visit(ProtoTypeAST *node) {
  std::string info;
  auto context = _module.SetContext(node->logger());

  // generate function prototype
  FuncPtr func;
  auto functions = _module.Functions();
  const auto& func_name = node->id();
  const auto& func_type = node->ast_type();
  if (!_module.GetFunction(func_name)) {
    func = _module.CreateFunction(func_name, func_type);
    functions.push_back(func);

    // add to environment
    auto symbols = _module.ValueSymTab();
    const auto &sym_table = _in_func ? symbols->outer() : symbols;
    sym_table->AddItem(func_name, func);
  } else {
    info = "function exist";
    return LogError(node->logger(), info);
  }

  // create entry block to init the parameters
  auto entry = _module.CreateBlock(func, "entry");
  _module.SetFuncEntry(entry);
  _module.SetInsertPoint(entry);
  std::size_t index = 0;
  for (const auto &it : node->params()) {
    auto arg_name = it->ArgName();
    auto param_alloc = it->CodeGeneAction(this);
    // set param_name
    std::static_pointer_cast<AllocaInst>(param_alloc)->set_name(arg_name + ".addr");
    auto arg_ref = _module.CreateArgRef(func, index++, arg_name);
    _module.CreateStore(arg_ref, param_alloc);
  }

  // create func_exit block to init the return value
  // TODO: not sure
  auto args_type = func_type->GetArgsType().value();
  auto ret_type = func_type->GetReturnType(args_type);
  auto ret_val = (ret_type->IsVoid()) ? nullptr : _module.CreateAlloca(ret_type);
  if (ret_val) std::static_pointer_cast<AllocaInst>(ret_val)->set_name("retval");
  _module.SetRetValue(ret_val);

  // jump to exit
  auto func_exit = _module.CreateBlock(func, "func_exit");
  _module.SetFuncExit(func_exit);
  return nullptr;
}

SSAPtr IRBuilder::visit(FunctionDefAST *node) {
  auto context = _module.SetContext(node->logger());

  // make new environment
  auto env = NewEnv();
  _in_func = true;

  // generate prototype
  auto prototype = node->header()->CodeGeneAction(this);

  // generate body
  auto body = node->body()->CodeGeneAction(this);

  auto func_exit = _module.FuncExit();
  _module.CreateJump(func_exit);
  _module.SetInsertPoint(func_exit);

  // create return value
  if (auto ret_val = _module.ReturnValue()) {
    auto ret = _module.CreateLoad(ret_val);
    _module.CreateReturn(ret);
  } else {
    _module.CreateReturn(nullptr);
  }

  return nullptr;
}

SSAPtr IRBuilder::visit(FuncParamAST *node) {
  auto cxt = _module.SetContext(node->logger());
  DBG_ASSERT(_in_func, "create parameter should in function");

  // create alloca
  auto param = _module.CreateAlloca(node->ast_type());

  // add param to symtab
  _module.ValueSymTab()->AddItem(node->ArgName(), param);
  return param;
}

SSAPtr IRBuilder::visit(PrimTypeAST *node) {
  return nullptr;
}


// create while block
/*
 *          |-----> cond --------|
 *          |        |           |
 *          |    loop_body       |
 *          |        |           |
 *          |--- true_block      |
 *                               |
 *               loop_end <------|
 *
 */
SSAPtr IRBuilder::visit(WhileStmt *node) {
  auto context = _module.SetContext(node->logger());
  auto cur_insert = _module.InsertPoint();  // save current insert point
  auto func = cur_insert->parent();

  // create while blocks
  auto cond_block = _module.CreateBlock(func, "while.cond");
  auto loop_body = _module.CreateBlock(func, "loop.body");
  auto while_end = _module.CreateBlock(func, "while.end");

  // TODO: handle break/continue
  _module.CreateJump(cond_block);
  _module.SetInsertPoint(cond_block);
  auto cond = node->cond()->CodeGeneAction(this);
  DBG_ASSERT(cond != nullptr, "while condition SSA is nullptr");

  _module.CreateBranch(cond, loop_body, while_end);

  // emit loop body
  _module.SetInsertPoint(loop_body);
  auto body = node->body()->CodeGeneAction(this);
  DBG_ASSERT(body != nullptr, "while body SSA is nullptr");
  _module.CreateJump(cond_block);

  _module.SetInsertPoint(while_end);

  return nullptr;
}

SSAPtr IRBuilder::visit(InitListAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(StructDefAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(EnumDefAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(StructElemAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(StructElemDefAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(TypeAliasAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(EnumElemAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(CastStmt *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(IndexAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(AccessAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(StructTypeAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(EnumTypeAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(ConstTypeAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(PointerTypeAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(UserTypeAST *) {
  return lava::mid::SSAPtr();
}

SSAPtr IRBuilder::visit(TranslationUnitDecl *node) {
  auto guard = NewEnv();
  for (const auto &it : node->decls()) {
    it->CodeGeneAction(this);
  }

  return nullptr;
}

// print error message
SSAPtr IRBuilder::LogError(const front::LoggerPtr &log,
                           std::string &message, const std::string &id) {
  log->LogError(message, id);
  return nullptr;
}

SSAPtr IRBuilder::LogError(const front::LoggerPtr &log, std::string &message) {
  log->LogError(message);
  return nullptr;
}


}