#include "irbuilder.h"
#include "common/casting.h"

#include <deque>
#include <iostream>

using namespace lava::define;

namespace lava::mid {

/* get array's linear length
 * int a[5][3] -> 5*3
 */
int GetLinearArrayLength(const TypePtr &ptr) {
  DBG_ASSERT(ptr->IsArray(), "not array type");
  TypePtr def = ptr;
  int len = ptr->GetLength();
  while ((def = def->GetDerefedType()) != nullptr) {
    int tmp = def->GetLength();
    len *= tmp ? tmp : 1;
  }
  return len;
}

/* get base element type
 * int a[3][4] -> int
 */
TypePtr GetArrayLinearBaseType(const TypePtr &type) {
  DBG_ASSERT(type->IsArray(), "not array type");
  TypePtr base_type = type->GetDerefedType();
  while (base_type->GetDerefedType()) {
    base_type = base_type->GetDerefedType();
  }
  return base_type;
}

SSAPtrList GetArrayInitElement(InitListAST *node, IRBuilder *irbuiler) {
  SSAPtrList elems;
  auto &module = irbuiler->module();

  /* Get current dimension size */
  auto &array_lens = module.array_lens();
  std::size_t len, cur = array_lens.front();
  array_lens.pop_front();

  int size = 1;

  // calculate the size of the current array
  if (!array_lens.empty()) {
    for (const auto &it : array_lens) {
      size *= it;
    }
  }

  len = cur * size;

  for (const auto &it : node->exprs()) {
    if (!it->ast_type()->IsArray()) {
      auto elem = it->CodeGeneAction(irbuiler);
      DBG_ASSERT(elem != nullptr, "emit array element failed");
      elems.push_back(std::move(elem));
    } else {
      auto tmp = GetArrayInitElement(static_cast<InitListAST *>(it.get()), irbuiler);
      elems.insert(elems.end(), tmp.begin(), tmp.end());
    }
  }

  // fill the init elements
  while (elems.size() < len) {
    auto zero = module.GetZeroValue(Type::Int32);
    elems.push_back(std::move(zero));
  }

  array_lens.push_front(cur);

  return elems;
}

void IRBuilder::SetInsertPointAtEntry() {
  auto func_entry = _module.FuncEntry();
  auto last = func_entry->inst_end();
  _module.SetInsertPoint(func_entry, --last);
}

void IRBuilder::SetInsertPoint(const BlockPtr &BB) {
  _module.SetInsertPoint(BB);
}

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
  auto context = _module.SetContext(node->logger());

  auto var_ssa = _module.GetValues(node->id());
  DBG_ASSERT(var_ssa != nullptr, "variable not found");

  auto varType = var_ssa->type();

  // create gep at array's first use
  if (varType->IsPointer() && varType->GetDerefedType()->IsArray()) {

    // gep at function entry first if expr is global array
    if (_module.IsGlobalVariable(var_ssa)) {
      // save global array's origin SSA
      _module.SaveOriginArray(node->id(), var_ssa);

      // update insert point to function entry
      auto cur_insert = _module.InsertPoint();
      SetInsertPointAtEntry();

      // generate array index: const zero
      SSAPtrList acc_index;
      auto zero = _module.GetZeroValue(Type::Int32);
      acc_index.push_back(zero);  // get array address
      acc_index.push_back(zero);  // get array head address

      // create gep, set is global copy
      var_ssa = _module.CreateElemAccess(var_ssa, acc_index);

      // recovery the insert point
      SetInsertPoint(cur_insert);

      // replace it with current gep
      _module.ReplaceValue(node->id(), var_ssa);
    }

  }

  return var_ssa;
}

SSAPtr IRBuilder::visit(VariableDecl *node) {
  auto context = _module.SetContext(node->logger());

  // save current insert point
  auto cur_insert = _module.InsertPoint();

  for (const auto &it : node->defs()) {
    it->CodeGeneAction(this);
  }

  _module.SetInsertPoint(cur_insert);

  return nullptr;
}

SSAPtr IRBuilder::visit(VariableDefAST *node) {
  auto context = _module.SetContext(node->logger());

  bool is_array = !node->arr_lens().empty();
  const auto &type = node->ast_type();
  SSAPtr variable;

  // global variable
  // TODO: need to handle default init
  if (_module.ValueSymTab()->is_root()) {
    GlobalVarPtr var = _module.CreateGlobalVar(!type->IsConst(), node->id(), type);
    if (node->hasInit()) {
      auto &init = node->init();
      DBG_ASSERT(init->IsLiteral(), "init value of global variable should be const expr");

      /* set array initlist dimension */
      if (!node->arr_lens().empty()) {
        std::deque<int> array_lens;
        TypePtr def = node->init()->ast_type();
        do {
          int len = def->GetLength();
          if (len == 0) break;
          array_lens.push_back(len);
        } while ((def = def->GetDerefedType()) != nullptr);
        _module.SetArrayLens(array_lens);
      }

      SSAPtr init_expr;
      auto init_ssa = init->CodeGeneAction(this);

      auto def = init_ssa->type()->GetDerefedType();
      if (def && def->IsArray()) {
        init_expr = init_ssa;
        var->set_type(init_ssa->type());

        DBG_ASSERT(init_expr != nullptr, "emit init of global variable failed");

        // set init array to nullptr is this array is all zero
        bool is_all_zero = true;
        for (const auto &init_value : (*dyn_cast<ConstantArray>(init_ssa))) {
          auto elem = dyn_cast<ConstantInt>(init_value.value());
          DBG_ASSERT(elem != nullptr, "elem is null ptr");
          if (!elem->IsZero()) {
            is_all_zero = false;
            break;
          }
        }
        if (is_all_zero) init_expr = nullptr;

      } else {
        init_expr = _module.CreateCastInst(init_ssa, type);
        DBG_ASSERT(init_expr != nullptr, "emit init of global variable failed");
      }


      var->set_init(init_expr);
    } else {
      if (type->IsArray()) {
        int array_len = GetLinearArrayLength(type);

        // get base element type
        TypePtr base_type = GetArrayLinearBaseType(type);

        // generate new type of linear array
        auto new_type = std::make_shared<ArrayType>(base_type, array_len, false);
        var->set_type(MakePointer(new_type));
      }
      // TODO: handle the zeroinitializer
    }
    variable = var;
  } else {
    // local variable

    // check if it is array
    if (!is_array) {
      variable = _module.CreateAlloca(type);
    }

    if (node->hasInit()) {

      /* set array initlist dimension */
      if (!node->arr_lens().empty()) {
        std::deque<int> array_lens;
        TypePtr def = node->init()->ast_type();
        do {
          int len = def->GetLength();
          if (len == 0) break;
          array_lens.push_back(len);
        } while ((def = def->GetDerefedType()) != nullptr);
        _module.SetArrayLens(array_lens);
      }

      auto init_ssa = node->init()->CodeGeneAction(this);
      DBG_ASSERT(init_ssa != nullptr, "emit init value failed");

      if (node->arr_lens().empty()) {
        _module.CreateAssign(variable, init_ssa);
      } else {
        variable = init_ssa;
      }
    } else {

      // create local array without init
      if (is_array) {
        int array_len = GetLinearArrayLength(type);
        // get base element type
        TypePtr base_type = GetArrayLinearBaseType(type);

        // alloca a linear array
        // generate new type of linear array
        auto new_type = std::make_shared<ArrayType>(base_type, array_len, false);

        auto val = _module.CreateAlloca(new_type);

        // emit gep instruction
        SSAPtrList index;
        auto zero = _module.GetZeroValue(Type::Int32);
        index.push_back(zero);
        index.push_back(zero);

        variable = _module.CreateElemAccess(val, index);
      }
    }
  }

  auto symtab = _module.ValueSymTab();
  symtab->AddItem(node->id(), variable);

  return nullptr;
}

SSAPtr IRBuilder::visit(InitListAST *node) {
  // update context
  auto context = _module.SetContext(node->logger());

  SSAPtr val;
  const auto &type = node->ast_type();
  bool is_root = _module.ValueSymTab()->is_root();
  std::size_t array_len = GetLinearArrayLength(type);

  // get base element type
  TypePtr base_type = GetArrayLinearBaseType(type);

  // create an array in rodata if its init list is const literal
  if (node->IsLiteral()) {
    ArrayPtr const_array;
    auto arr_name = _module.GetArrayName();

    bool isEmpty = false;
    SSAPtrList exprs;

    /* handle empty initial list */
    if (node->exprs().empty()) {
      isEmpty = true;

      // init array with const zero

      for (std::size_t i = 0; i < array_len; i++) {
        auto expr = _module.GetZeroValue(define::Type::Int32);
        exprs.push_back(std::move(expr));
      }

      // generate constant array
      DBG_ASSERT(type->IsArray(), "type is not array");

      /* create a global const array if this is a global array */
      // add to global symbol table
      if (is_root) {
        // generate new type of linear array
        auto new_type = std::make_shared<ArrayType>(base_type, array_len, false);
        const_array = _module.CreateArray(exprs, new_type, arr_name);
        return const_array;
      }
    } else {
      /* Handle non-empty initial list */

      // generate all element
      exprs = GetArrayInitElement(node, this);

      // generate constant array
      DBG_ASSERT(type->IsArray(), "type is not array");

      /* create a global const array if this is a global array */
      // add to global symbol table
      if (is_root) {
        // generate new type of linear array
        auto new_type = std::make_shared<ArrayType>(base_type, array_len, false);
        const_array = _module.CreateArray(exprs, new_type, arr_name);
        return const_array;
      }
    }
#if 0
    /* Get element address from global copy array */

    // store into global vairable
    _module.GlobalVars().push_back(const_array);

    /* create a gep at the function entry */
    // save current insert point and set point as function entry
    auto cur_insert = _module.InsertPoint();
    SetInsertPointAtEntry();

    // emit gep instruction
    SSAPtrList index;
    auto zero = _module.GetZeroValue(Type::Int32);
    index.push_back(zero);
    index.push_back(zero);

    val = _module.CreateElemAccess(const_array, index);

    // recover the insert point
    SetInsertPoint(cur_insert);

    return val;
#endif

    /* alloca a memory on stack for array */

    // create a linear array on stack
    auto new_type = std::make_shared<ArrayType>(base_type, array_len, false);
    val = _module.CreateAlloca(new_type);

    // create zero
    auto zero = _module.GetZeroValue(Type::Int32);

    // generate array length TODO: dirty hack
    auto length = _module.CreateConstInt(array_len * 4);

    // convert array pointer to i32*
    // emit gep instruction
    SSAPtrList index;
    zero = _module.GetZeroValue(Type::Int32);
    index.push_back(zero);  // TODO: the same const, unknown if it has side effect on use-def
    index.push_back(zero);

    val = _module.CreateElemAccess(val, index);

    // memset with zero
    std::vector<SSAPtr> args;
    auto memset_ssa = _module.GetFunction("memset");

    // set arguments
    args.push_back(val);     // array address
    args.push_back(zero);    // array value
    args.push_back(length);  // array length
    _module.CreateCallInst(memset_ssa, args);

    // if is empty then return
    if (isEmpty) return val;

    // copy init value
    auto it = exprs.begin();
    for (std::size_t i = 0; i < exprs.size(); i++) {
      if (!(*it)->IsConst() || !dyn_cast<ConstantInt>(*it)->IsZero()) {
        auto ptr = _module.CreateElemAccess(val, SSAPtrList{_module.CreateConstInt(i)});
        _module.CreateAssign(ptr, *it);
      }
      it++;
    }



    return val;
  } else {
    /* create a temporary alloca */

    // alloca a linear array
    // generate new type of linear array
    auto new_type = std::make_shared<ArrayType>(base_type, array_len, false);

    val = _module.CreateAlloca(new_type);

    // emit gep instruction
    SSAPtrList index;
    auto zero = _module.GetZeroValue(Type::Int32);
    index.push_back(zero);
    index.push_back(zero);

    val = _module.CreateElemAccess(val, index);

    // generate array length TODO: dirty hack
    auto length = _module.CreateConstInt(array_len * 4);

    // memset with zero
    std::vector<SSAPtr> args;
    auto memset_ssa = _module.GetFunction("memset");

    // set arguments
    args.push_back(val);     // array address
    args.push_back(zero);    // array value
    args.push_back(length);  // array length
    _module.CreateCallInst(memset_ssa, args);

    // generate elements
    SSAPtrList exprs = GetArrayInitElement(node, this);
    auto it = exprs.begin();
    for (std::size_t i = 0; i < exprs.size(); i++) {
      if (!(*it)->IsConst() || !dyn_cast<ConstantInt>(*it)->IsZero()) {
        auto ptr = _module.CreateElemAccess(val, SSAPtrList{_module.CreateConstInt(i)});
        _module.CreateAssign(ptr, *it);
      }
      it++;
    }

    for (std::size_t i = exprs.size(); i < node->exprs().size(); i++) {
      auto elem = node->exprs()[i]->CodeGeneAction(this);
      auto ptr = _module.CreateElemAccess(val, SSAPtrList{_module.CreateConstInt(i)});
      _module.CreateAssign(ptr, elem);
    }

    return val;
  }

}

SSAPtr IRBuilder::visit(BinaryStmt *node) {
  auto lhs = node->lhs()->CodeGeneAction(this);
  DBG_ASSERT(lhs != nullptr, "lhs generate failed");
  SSAPtr rhs = nullptr;
  SSAPtr bin_inst = nullptr;


  TypePtr type = nullptr;
  if (lhs->type()->IsPointer()) {
    type = lhs->type()->GetDerefedType();
  } else {
    type = lhs->type();
  }

  const auto &func = _module.InsertPoint()->parent();
  auto zero = _module.GetZeroValue(type->GetType());
  if (node->op() == front::Operator::LAnd) {
    bin_inst = _module.CreateAlloca(type);
    auto lhs_true = _module.CreateBlock(func, "lhs.true");
    auto lhs_false = _module.CreateBlock(func, "lhs.false");
    auto land_end = _module.CreateBlock(func, "land.end");
    auto cond = _module.CreateICmpInst(BinaryStmt::Operator::NotEqual, zero, lhs);
    _module.CreateBranch(cond, lhs_true, lhs_false);

    /* LHS true */
    // generate rhs in lhs_true block
    _module.SetInsertPoint(lhs_true);
    rhs = node->rhs()->CodeGeneAction(this);
    DBG_ASSERT(rhs != nullptr, "rhs generate failed");

    const auto &lty = lhs->type();
    const auto &rty = rhs->type();
    SSAPtr LHS = lhs, RHS = rhs;
    if (lty->IsInteger() && rty->IsInteger()) {
      const auto &ty = GetCommonType(lty, rty);
      LHS = _module.CreateCastInst(lhs, ty);
      RHS = _module.CreateCastInst(rhs, ty);
    }

    // create land statement
    auto landInst = _module.CreateBinaryOperator(node->op(), LHS, RHS);
    DBG_ASSERT(landInst != nullptr, "logic-and statement generate failed");

    // save result
    _module.CreateStore(landInst, bin_inst);
    // jump to land end
    _module.CreateJump(land_end);

    /* LHS false */
    // save rhs value
    _module.SetInsertPoint(lhs_false);
    if (lhs->type()->IsPointer()) {
      lhs = _module.CreateLoad(lhs);
    }
    _module.CreateStore(lhs, bin_inst);
    // jump to land end
    _module.CreateJump(land_end);

    _module.SetInsertPoint(land_end);

    // load result
    bin_inst = _module.CreateLoad(bin_inst);

  } else if (node->op() == front::Operator::LOr) {
    bin_inst = _module.CreateAlloca(type);
    auto lhs_true = _module.CreateBlock(func, "lhs.true");
    auto lhs_false = _module.CreateBlock(func, "lhs.false");
    auto lor_end = _module.CreateBlock(func, "lor.end");
    auto cond = _module.CreateICmpInst(BinaryStmt::Operator::NotEqual, zero, lhs);
    _module.CreateBranch(cond, lhs_true, lhs_false);

    /* LHS false */
    _module.SetInsertPoint(lhs_false);
    rhs = node->rhs()->CodeGeneAction(this);
    DBG_ASSERT(rhs != nullptr, "rhs generate failed");

    const auto &lty = lhs->type();
    const auto &rty = rhs->type();
    SSAPtr LHS = lhs, RHS = rhs;
    if (lty->IsInteger() && rty->IsInteger()) {
      const auto &ty = GetCommonType(lty, rty);
      LHS = _module.CreateCastInst(lhs, ty);
      RHS = _module.CreateCastInst(rhs, ty);
    }

    // create lor statement
    auto lorInst = _module.CreateBinaryOperator(node->op(), LHS, RHS);
    DBG_ASSERT(lorInst != nullptr, "logic-or statement generate failed");

    // save result
    _module.CreateStore(lorInst, bin_inst);

    // jump to land end
    _module.CreateJump(lor_end);

    /* LHS true */
    // computer will not execute rhs if lhs is true
    _module.SetInsertPoint(lhs_true);
    // save rhs value
    if (lhs->type()->IsPointer()) {
      lhs = _module.CreateLoad(lhs);
    }
    _module.CreateStore(lhs, bin_inst);
    // jump to lor end
    _module.CreateJump(lor_end);

    _module.SetInsertPoint(lor_end);

    // create load
    bin_inst = _module.CreateLoad(bin_inst);
  } else {
    rhs = node->rhs()->CodeGeneAction(this);
    DBG_ASSERT(rhs != nullptr, "rhs generate failed");

    const auto &lty = lhs->type();
    const auto &rty = rhs->type();
    SSAPtr LHS = lhs, RHS = rhs;
    if (lty->IsInteger() && rty->IsInteger()) {
      const auto &ty = GetCommonType(lty, rty);
      LHS = _module.CreateCastInst(lhs, ty);
      RHS = _module.CreateCastInst(rhs, ty);
    }

    bin_inst = _module.CreateBinaryOperator(node->op(), LHS, RHS);
    DBG_ASSERT(bin_inst != nullptr, "binary statement generate failed");
  }

  return bin_inst;
}

SSAPtr IRBuilder::visit(UnaryStmt *node) {
  using Op = front::Operator;
  auto context = _module.SetContext(node->logger());
  auto opr = node->opr()->CodeGeneAction(this);

  TypePtr type = nullptr;
  if (opr->type()->IsPointer()) {
    type = opr->type()->GetDerefedType();
  } else {
    type = opr->type();
  }
  switch (node->op()) {
    case Op::Pos:
      return opr;
    case Op::Neg: {
      auto zero = _module.GetZeroValue(type->GetType());
      auto res = _module.CreateBinaryOperator(Op::Sub, zero, opr);
      return res;
    }
    case Op::Not: {
      auto allBitsOne = GetAllOneValue(type->GetType());
      auto res = _module.CreateBinaryOperator(Op::Xor, opr, allBitsOne);
      return res;
    }
    case Op::LNot: {
      // TODO: didn't implement
      SSAPtr tobool = opr;
      if (opr->type()->IsInteger() || (opr->type()->GetDerefedType() && opr->type()->GetDerefedType()->IsInteger())) {
        auto zero = _module.GetZeroValue(type->GetType());
        tobool = _module.CreateICmpInst(Op::NotEqual, zero, opr);
      }
      auto trueConst = _module.CreateConstInt(1, Type::Bool);
      auto lnot = _module.CreateBinaryOperator(Op::Xor, trueConst, tobool);
      return lnot;
    }
    default:
      DBG_ASSERT(0, "not unary operator");
      return nullptr;
  }
}

// TODO:: emit control stmts
SSAPtr IRBuilder::visit(ControlAST *node) {
  auto context = _module.SetContext(node->logger());

  // create a new block
  const auto &func = _module.InsertPoint()->parent();
  auto new_block = _module.CreateBlock(func);

  switch (node->type()) {

    case ControlAST::Type::Break:
    case ControlAST::Type::Continue: {
      // generate target
      const auto &bk_cont = _module.BreakCont().top();
      auto target = node->type() == ControlAST::Type::Break ? bk_cont.first : bk_cont.second;

      // jump to target block
      _module.CreateJump(target);
      break;
    }
    case ControlAST::Type::Return: {
      if (node->expr()) {
        // generate return value
        auto retval = node->expr()->CodeGeneAction(this);
        DBG_ASSERT(retval != nullptr, "emit return value failed");

        // copy return value
        auto store_inst = _module.CreateAssign(_module.ReturnValue(), retval);
        DBG_ASSERT(store_inst != nullptr, "copy return value failed");
      }

      auto jump_inst = _module.CreateJump(_module.FuncExit());
      DBG_ASSERT(jump_inst != nullptr, "emit jump instruction failed");

      break;
    }
  }
  // set insert point at new block
  _module.SetInsertPoint(new_block);
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

  // create if-else blocks
  auto &cond = node->cond();
  auto then_block = _module.CreateBlock(func, "if.then");
  auto else_block = _module.CreateBlock(func, "if.else");
  auto end_block = _module.CreateBlock(func, "if.end");

  auto cond_block = cond->CodeGeneAction(this);
  DBG_ASSERT(cond_block != nullptr, "emit condition statement failed");
  _module.CreateBranch(cond_block, then_block, else_block);

  // create then block
  auto &then_ast = node->then();
  _module.SetInsertPoint(then_block);
  auto then_ssa = then_ast->CodeGeneAction(this);
  _module.CreateJump(end_block);

  // create else block
  _module.SetInsertPoint(else_block);
  if (node->hasElse()) {
    auto &else_ast = node->else_then();
    auto else_ssa = else_ast->CodeGeneAction(this);
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
  const auto &func_name = node->id();
  const auto &func_type = node->ast_type();
  if (!_module.GetFunction(func_name)) {

    func = _module.CreateFunction(func_name, func_type, !_in_func);
    functions.push_back(func);

    // add to environment
    auto symbols = _module.ValueSymTab();
    const auto &sym_table = _in_func ? symbols->outer() : symbols;
    sym_table->AddItem(func_name, func);
  } else {
    info = "function exist";
    return LogError(node->logger(), info);
  }

  // return if is just a declaration
  if (!_in_func) return nullptr;

  // create entry block to init the parameters
  auto entry = _module.CreateBlock(func, "entry");
  _module.SetFuncEntry(entry);
  _module.SetInsertPoint(entry);
  std::size_t index = 0;
  for (const auto &it : node->params()) {
    auto arg_name = it->ArgName();
    auto param_alloc = it->CodeGeneAction(this);
    // set param_name
    auto arg_ref = _module.CreateArgRef(func, index++, arg_name);
    _module.CreateStore(arg_ref, param_alloc);

    // convert it to i32*
    if (param_alloc->type()->GetDerefedType()->GetDerefedType()) {
      auto loadInst = _module.CreateLoad(param_alloc);

      _module.ReplaceValue(it->ArgName(), loadInst);
    } else {
      std::static_pointer_cast<AllocaInst>(param_alloc)->set_name(arg_name + ".addr");
    }
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

  // recover the changed global array SSA
  _module.RecoverArrays();

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
  // Add to break/continue stack
  _module.BreakCont().push({while_end, cond_block});
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

  // update insert point to end block
  _module.SetInsertPoint(while_end);

  // pop the top element of break/continue stack
  _module.BreakCont().pop();
  return nullptr;
}

SSAPtr IRBuilder::visit(IndexAST *node) {
  auto context = _module.SetContext(node->logger());

  // generate expression & index
  auto expr = node->expr()->CodeGeneAction(this);
  DBG_ASSERT(expr != nullptr, "emit expr for accessing failed");

  auto index = node->index()->CodeGeneAction(this);
  DBG_ASSERT(index != nullptr, "emit index for accessing failed");
  if (!index->type()->IsConst() && !IsBinaryOperator(index) && !IsCallInst(index)) {
    index = _module.CreateLoad(index);
    DBG_ASSERT(index != nullptr, "emit load index failed");
  }

  // get type
  auto expr_ty = node->expr()->ast_type();
  auto elem_ty = expr_ty->GetDerefedType();

  SSAPtr ptr;


  if (expr_ty->GetDerefedType()->IsArray()) {
    // get rest length of the array
    auto derefType = expr_ty->GetDerefedType();
    int dim_len = GetLinearArrayLength(derefType);
    SSAPtr dim_len_ssa = _module.CreateConstInt(dim_len);

    index = _module.CreateBinaryOperator(BinaryStmt::Operator::Mul, dim_len_ssa, index);
  }

  if (index->isInstruction()) {
    if (index->type()->IsPointer()) {
      index = _module.CreateLoad(index);
    }
  }

  // update array's type
  elem_ty = expr->type()->GetDerefedType();

  ptr = _module.CreateElemAccess(expr, SSAPtrList{index});

  DBG_ASSERT(ptr != nullptr, "emit index failed");
  return ptr;
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