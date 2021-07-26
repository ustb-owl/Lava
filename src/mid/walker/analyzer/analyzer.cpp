#include "mid/walker/analyzer/analyzer.h"

#include <cassert>

using namespace lava::mid;
using namespace lava::define;
using namespace lava::front;

namespace {

// print error message
inline TypePtr LogError(const LoggerPtr &log, std::string_view message) {
  log->LogError(message);
  return nullptr;
}

// print error message (with specific identifier)
inline TypePtr LogError(const LoggerPtr &log, std::string_view message,
                        std::string_view id) {
  log->LogError(message, id);
  return nullptr;
}

// check value initializing (variable definition, function returning)
inline bool CheckInit(const LoggerPtr &log, const TypePtr &type,
                      const TypePtr &init, std::string_view id) {
  assert(!type->IsRightValue());
  bool ret = type->IsConst() || type->IsArray() ? type->IsIdentical(init)
                                                : type->CanAccept(init);
  if (!ret) {
    if (id.empty()) {
      log->LogError("type mismatch when initializing");
    } else {
      log->LogError("type mismatch when initializing", id);
    }
  }
  return ret;
}

}  // namespace

ASTPtrList Analyzer::GetLinearInitList(ASTPtrList &initList) {
  ASTPtrList result;

  // update dimension information
  int current_dim = array_lens_.front();
  array_lens_.pop_front();

  // get current dim size
  int rest_size = 1;
  for (const auto &it : array_lens_) rest_size *= it;

  // get current dim size
  int current_size = current_dim * rest_size;

  auto init = initList.begin();
  while (true) {
    if (init == initList.end()) break;

    if ((*init)->ast_type()->IsInteger()) {
      int loc = result.size();
      for (int size = result.size(); size < loc + rest_size; size++) {
        if ((init != initList.end()) && (*init)->ast_type()->IsInteger()) {
          result.push_back(std::move(*init));
          init++;
        } else {
          auto zero = std::make_unique<IntAST>(0);
          zero->set_ast_type(MakePrimType(Type::Int32, true));
          result.push_back(std::move(zero));
        }
      }
//      break;
    }

    if (init == initList.end()) break;
    if ((*init)->ast_type()->IsArray()) {
      auto res = GetLinearInitList(static_cast<InitListAST *>((*init).get())->exprs());
      DBG_ASSERT(res.size() == rest_size, "size of sub list is incorrect");

      // concat result
      result.insert(
          result.end(),
          std::make_move_iterator(res.begin()),
          std::make_move_iterator(res.end())
      );
      init++;
    }
  }

  // padding with 0
  while ((int)result.size() < current_size) {
    auto zero = std::make_unique<IntAST>(0);
    zero->set_ast_type(MakePrimType(Type::Int32, true));
    result.push_back(std::move(zero));
  }
  DBG_ASSERT(result.size() == current_size, "size of list is incorrect");

  array_lens_.push_front(current_dim);
  return result;
}

ASTPtrList Analyzer::ListToMatrix(std::deque<int> dims, ASTPtrList &initList, bool is_top) {
  auto current_dim = dims.front();
  dims.pop_front();

  ASTPtrList result;
  if (dims.empty()) {
    int count = 0;
    for (auto it = initList.begin(); it != initList.end(); ) {
      ASTPtrList tmp;
      for (int i = 0; i < current_dim; i++, it++, count++) {
        tmp.push_back(std::move(*it));
      }
      DBG_ASSERT(tmp.size() == current_dim, "tmp list size is incorrect");

      auto initListNode = std::make_unique<InitListAST>(std::move(tmp));
      auto type = std::make_shared<ArrayType>(MakePrimType(Type::Int32, true), current_dim , true);
      initListNode->set_ast_type(type);
      result.push_back(std::move(initListNode));
    }

    DBG_ASSERT(count == initList.size(), "didn't handle all initList elements");
  } else {
    int count = 0;
    auto sub = ListToMatrix(dims, initList);

    if (is_top) return sub;

    for (auto it = sub.begin(); it != sub.end();) {
      ASTPtrList tmp;
      for (int i = 0; i < current_dim; i++, it++, count++) {
        tmp.push_back(std::move(*it));
      }
      DBG_ASSERT(tmp.size() == current_dim, "tmp list size is incorrect");

      auto initListNode = std::make_unique<InitListAST>(std::move(tmp));

      // make type
      auto sub_type = initListNode->exprs()[0]->ast_type()->GetValueType(true);
      DBG_ASSERT(sub_type != nullptr, "create sub type failed");
      auto type = std::make_shared<ArrayType>(sub_type, current_dim, true);
      initListNode->set_ast_type(type);
      result.push_back(std::move(initListNode));
    }

    DBG_ASSERT(count == sub.size(), "didn't handle all sub list elements");
  }

  return result;
}

// definition of static properties
TypePtr Analyzer::enum_base_ = MakePrimType(Type::Int32, false);

xstl::Guard Analyzer::NewEnv() {
  symbols_ = xstl::MakeNestedMap(symbols_);
  aliases_ = xstl::MakeNestedMap(aliases_);
  structs_ = xstl::MakeNestedMap(structs_);
  enums_ = xstl::MakeNestedMap(enums_);
  return xstl::Guard([this] {
    symbols_ = symbols_->outer();
    aliases_ = aliases_->outer();
    structs_ = structs_->outer();
    enums_ = enums_->outer();
  });
}

TypePtr Analyzer::HandleArray(TypePtr base, const ASTPtrList &arr_lens,
                              std::string_view id, bool is_param) {

  for (int i = arr_lens.size() - 1; i >= 0; --i) {
    const auto &expr = arr_lens[i];
    // analyze expression
    if (expr) {
      auto expr_ty = expr->SemaAnalyze(*this);
      if (!expr_ty || !expr_ty->IsInteger()) {
        return LogError(expr->logger(), "integer required");
      }
    }
    // create array type
    if (is_param && (!expr || !i)) {
      // check error
      if (!expr && i) {
        return LogError(expr->logger(), "incomplete array type", id);
      }
      // make a pointer
      base = MakePointer(base, false);
    } else {
      // try to evaluate current dimension
      auto len = expr->Eval(eval_);
      if (!len || !*len) {
        return LogError(expr->logger(), "invalid array length", id);
      }
      // make array type
      base = std::make_shared<ArrayType>(std::move(base), *len, false);
    }
  }

  if (base->IsArray()) {
    DBG_ASSERT(is_top_dim_ == false, "top dim flag is incorrect");
    is_top_dim_ = true;
    array_lens_.clear();
    auto it = base;
    while (it->GetDerefedType()) {
      array_lens_.push_back(it->GetLength());
      it = it->GetDerefedType();
    }

    DBG_ASSERT(arr_lens.size() == array_lens_.size(), "get array length failed");
  }

  return base;
}

void Analyzer::Reset() {
  auto new_env = [] {
    return xstl::MakeNestedMap<std::string, TypePtr>();
  };
  symbols_ = new_env();
  aliases_ = new_env();
  structs_ = new_env();
  enums_ = new_env();
  assert(final_types_.empty());
  in_func_ = false;
  funcs_.clear();
  in_loop_ = 0;
}

TypePtr Analyzer::AnalyzeOn(TranslationUnitDecl &ast) {
  // make new environment when not in function
  auto guard = !in_func_ ? NewEnv() : xstl::Guard(nullptr);

  // analyze declarations and definitions
  for (const auto &i : ast.decls()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }

  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(VariableDecl &ast) {
  // get type & check
  var_type_ = ast.type()->SemaAnalyze(*this);
  if (!var_type_) return nullptr;
  if (var_type_->IsVoid()) {
    return LogError(ast.type()->logger(), "variable can not be void type");
  }
  // handle definitions
  for (const auto &i : ast.defs()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  // evaluate current AST
  ast.Eval(eval_);
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(VariableDefAST &ast) {
  // handle array type
  auto type = HandleArray(var_type_, ast.arr_lens(), ast.id(), false);
  if (!type) return nullptr;
  // push to stack in order to handle initializer list
  final_types_.push(type);
  auto guard = xstl::Guard([this] { final_types_.pop(); });
  // check type of initializer
  if (ast.init()) {
    const auto &log = ast.init()->logger();
    auto init = ast.init()->SemaAnalyze(*this);
    if (!init) return nullptr;
    if (!CheckInit(log, type, init, ast.id())) return nullptr;
  } else {
    is_top_dim_ = false;
  }
  // check if is conflicted
  if (symbols_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "symbol has already been defined",
                    ast.id());
  }
  // add to environment
  symbols_->AddItem(ast.id(), type);
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(InitListAST &ast) {

  auto getArrayLinearBaseType = [](const TypePtr &type) -> TypePtr {
    DBG_ASSERT(type->IsArray(), "not array type");
    TypePtr base_type = type->GetDerefedType();
    while (base_type->GetDerefedType()) {
      base_type = base_type->GetDerefedType();
    }
    return base_type;
  };

  auto &exprs = ast.exprs();
  const auto &type = final_types_.top();
  auto base_type = getArrayLinearBaseType(type);
  DBG_ASSERT(!array_lens_.empty(), "arrary length list is empty");

  // get type
  bool is_top_dim = is_top_dim_;
  is_top_dim_ = false;
  for (const auto &it : ast.exprs()) {
    it->SemaAnalyze(*this);
  }


  /* ---------- rebuild linear array ---------- */
  if (is_top_dim) {
    // get total size
    int total_size = 1;
    for (const auto &it : array_lens_) total_size *= it;

    // get linear array list
    auto result = GetLinearInitList(exprs);
    DBG_ASSERT(total_size = result.size(), "init list size is incorrect");

    ASTPtrList final_init_list = std::move(result);
    if (array_lens_.size() > 1) {
      auto matrix = ListToMatrix(array_lens_, final_init_list, true);
      final_init_list = std::move(matrix);
    }
    ast.set_exprs(std::move(final_init_list));
  } else {
    return ast.set_ast_type(type->GetValueType(true));
  }

  return ast.set_ast_type(type->GetValueType(true));


#if 0
  // NOTE: this process will rebuild initializer list. what this process
  //       does is NOT quite same as what normal C/C++ compilers do.
  const auto &type = final_types_.top();
  assert(type->IsArray());
  // traverse array elements
  ASTPtrList new_exprs;
  auto &exprs = ast.exprs();
  auto it = exprs.begin();
  for (std::size_t i = 0; i < type->GetLength() && it != exprs.end(); ++i) {
    // get current element type
    TypePtr elem = type->GetElem(i), expr;
    final_types_.push(elem);
    auto guard = xstl::Guard([this] { final_types_.pop(); });
    // check if need to rebuild
    if (!(*it)->IsInitList() && elem->IsArray()) {
      // create a new initializer list
      ASTPtrList sub_exprs;
      for (std::size_t j = 0; j < elem->GetLength() &&
                              it != exprs.end() && !(*it)->IsInitList();
           ++j, ++it) {
        sub_exprs.push_back(std::move(*it));
      }
      auto sub_list = std::make_unique<InitListAST>(std::move(sub_exprs));
      sub_list->set_logger(ast.logger());
      // analyze sub list
      expr = sub_list->SemaAnalyze(*this);
      new_exprs.push_back(std::move(sub_list));
    } else {
      // get expression type
      expr = (*it)->SemaAnalyze(*this);
      new_exprs.push_back(std::move(*it));
      ++it;
    }
    // check expression type
    if (!expr || !CheckInit(ast.logger(), elem, expr, "")) return nullptr;
  }
  // log warning
  if (it != exprs.end()) {
    ast.logger()->LogWarning("excess elements in initializer list");
  }
  // reset expressions
  ast.set_exprs(std::move(new_exprs));
  return ast.set_ast_type(type->GetValueType(true));
#endif
}

TypePtr Analyzer::AnalyzeOn(ProtoTypeAST &ast) {
  // get return type
  auto ret = ast.type()->SemaAnalyze(*this);
  if (!ret) return nullptr;
  if (in_func_) cur_ret_ = ret;
  // get type of parameters
  TypePtrList params;
  for (const auto &i : ast.params()) {
    auto param = i->SemaAnalyze(*this);
    if (!param) return nullptr;
    params.push_back(std::move(param));
  }
  // make function type
  auto type = std::make_shared<FuncType>(std::move(params),
                                         std::move(ret), true);
  // add to environment
  const auto &sym = in_func_ ? symbols_->outer() : symbols_;
  if (sym->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "symbol has already been defined",
                    ast.id());
  }
  sym->AddItem(ast.id(), type);
  // add to func info map
  auto it = funcs_.find(ast.id());
  if (it == funcs_.end()) {
    funcs_.insert({ast.id(), {type, !in_func_}});
  } else if (!it->second.type->IsIdentical(type)) {
    return LogError(ast.logger(), "conflicted function type", ast.id());
  } else if (!it->second.is_decl && in_func_) {
    return LogError(ast.logger(), "redefinition of function", ast.id());
  } else if (it->second.is_decl && in_func_) {
    it->second.is_decl = false;
  }
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(FunctionDefAST &ast) {
  // make new environment
  /*
    NOTE: structure of function's environment
          outer           <- global env
          |
          +- args/block   <- current env
  */
  auto env = NewEnv();
  // set flag, this flag will be cleared when entering body
  in_func_ = true;
  // register function & parameters
  auto func = ast.header()->SemaAnalyze(*this);
  if (!func) return nullptr;
  // analyze body
  if (!ast.body()->SemaAnalyze(*this)) return nullptr;
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(FuncParamAST &ast) {
  // get type
  auto type = ast.type()->SemaAnalyze(*this);
  if (!type) return nullptr;
  // handle array type
  type = HandleArray(std::move(type), ast.arr_lens(), ast.ArgName(), true);
  if (!type) return nullptr;
  // add to environment
  if (in_func_) {
    // check if is conflicted
    if (symbols_->GetItem(ast.ArgName(), false)) {
      return LogError(ast.logger(), "argument has already been declared",
                      ast.ArgName());
    }
    symbols_->AddItem(ast.ArgName(), type);
  }
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(StructDefAST &ast) {
  // reset status
  last_struct_name_ = ast.id();
  struct_elems_.clear();
  struct_elem_names_.clear();
  // create an empty struct type
  auto type = std::make_shared<StructType>(struct_elems_, ast.id(), false);
  // check if is conflicted
  if (structs_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "struct has already been defined",
                    ast.id());
  }
  // add to environment
  structs_->AddItem(ast.id(), type);
  // get type of elements
  for (const auto &i : ast.elems()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  // update struct type
  // TODO: circular reference!
  type->set_elems(std::move(struct_elems_));
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(EnumDefAST &ast) {
  // analyze elements
  for (const auto &i : ast.elems()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  // check if is conflicted
  if (enums_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "enumeration has already been defined",
                    ast.id());
  }
  // add to environment
  enums_->AddItem(ast.id(), enum_base_);
  // evaluate current AST
  ast.Eval(eval_);
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(TypeAliasAST &ast) {
  // get type
  auto type = ast.type()->SemaAnalyze(*this);
  if (!type) return nullptr;
  // check if is conflicted
  if (aliases_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "user type has already been defined",
                    ast.id());
  }
  // add to environment
  enums_->AddItem(ast.id(), type);
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(StructElemAST &ast) {
  // get base type
  auto base = ast.type()->SemaAnalyze(*this);
  if (!base) return nullptr;
  // check if is recursive type
  if (base->IsStruct() && base->GetTypeId() == last_struct_name_) {
    return LogError(ast.logger(), "recursive type is not allowed");
  }
  struct_elem_base_ = base;
  // analyze definitions
  for (const auto &i : ast.defs()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(StructElemDefAST &ast) {
  // check if name conflicted
  if (!struct_elem_names_.insert(ast.id()).second) {
    return LogError(ast.logger(), "conflicted struct element name",
                    ast.id());
  }
  // handle array type
  auto type = HandleArray(struct_elem_base_, ast.arr_lens(),
                          ast.id(), false);
  if (!type) return nullptr;
  // add to elements
  struct_elems_.push_back({std::string(ast.id()), type});
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(EnumElemAST &ast) {
  // check initializer
  const auto &expr = ast.expr();
  if (expr) {
    auto init = expr->SemaAnalyze(*this);
    if (!init || !enum_base_->CanAccept(init)) {
      return LogError(expr->logger(), "invalid enumerator initializer");
    }
  }
  // check if is conflicted
  if (symbols_->GetItem(ast.id(), false)) {
    return LogError(ast.logger(), "enumerator has already been defined",
                    ast.id());
  }
  // add to environment
  symbols_->AddItem(ast.id(), enum_base_->GetValueType(true));
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(CompoundStmt &ast) {
  // make new environment when not in function
  auto guard = !in_func_ ? NewEnv() : xstl::Guard(nullptr);
  if (in_func_) in_func_ = false;
  // ananlyze statements
  for (const auto &i : ast.stmts()) {
    if (!i->SemaAnalyze(*this)) return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(IfElseStmt &ast) {
  // analyze condition
  auto cond = ast.cond()->SemaAnalyze(*this);
  if (!cond || !(cond->IsInteger() || cond->IsPointer())) {
    return LogError(ast.cond()->logger(),
                    "condition must be an integer or a pointer");
  }
  // analyze branches
  if (!ast.then()->SemaAnalyze(*this)) return nullptr;
  if (ast.else_then() && !ast.else_then()->SemaAnalyze(*this)) {
    return nullptr;
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(WhileStmt &ast) {
  // analyze condition
  auto cond = ast.cond()->SemaAnalyze(*this);
  if (!cond || !(cond->IsInteger() || cond->IsPointer())) {
    return LogError(ast.cond()->logger(),
                    "condition must be an integer or a pointer");
  }
  // analyze body
  ++in_loop_;
  if (!ast.body()->SemaAnalyze(*this)) return nullptr;
  --in_loop_;
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(ControlAST &ast) {
  using Type = ControlAST::Type;
  switch (ast.type()) {
    case Type::Break:
    case Type::Continue: {
      // check if is in a loop
      if (!in_loop_) {
        return LogError(ast.logger(),
                        "using break/continue outside the loop");
      }
      break;
    }
    case Type::Return: {
      assert(cur_ret_->IsVoid() || !cur_ret_->IsRightValue());
      // check if is compatible
      if (ast.expr()) {
        auto ret = ast.expr()->SemaAnalyze(*this);
        if (!ret || !CheckInit(ast.expr()->logger(), cur_ret_, ret, "")) {
          return nullptr;
        }
      }
      break;
    }
    default:
      assert(false);
  }
  return ast.set_ast_type(MakeVoid());
}

TypePtr Analyzer::AnalyzeOn(BinaryStmt &ast) {
  using Op = front::Operator;
  // get lhs & rhs
  auto lhs = ast.lhs()->SemaAnalyze(*this);
  auto rhs = ast.rhs()->SemaAnalyze(*this);
  if (!lhs || !rhs) return nullptr;
  // preprocess some types
  if (lhs->IsVoid() || rhs->IsVoid()) {
    return LogError(ast.logger(), "invalid operation between void types");
  }
  // handle by operator
  TypePtr type;
  switch (ast.op()) {
    case Op::Add:
    case Op::Sub:
    case Op::SLess:
    case Op::SLessEq:
    case Op::SGreat:
    case Op::SGreatEq: {
      if (lhs->IsPointer() || rhs->IsPointer()) {
        // pointer operation
        if (lhs->IsPointer() && rhs->IsInteger()) {
          type = lhs;
        } else if (rhs->IsPointer() && lhs->IsInteger() &&
                   ast.op() != Op::Sub) {
          type = rhs;
        } else {
          return LogError(ast.logger(), "invalid pointer operation");
        }
        break;
      }
      // fall through
    }
    case Op::Mul:
    case Op::SDiv:
    case Op::SRem:
    case Op::And:
    case Op::Or:
    case Op::Xor:
    case Op::Shl:
    case Op::LShr:
    case Op::LAnd:
    case Op::LOr: {
      // int binary operation
      if (lhs->IsInteger() && rhs->IsInteger()) {
        type = GetCommonType(lhs, rhs);
      }
      break;
    }
    case Op::Equal:
    case Op::NotEqual: {
      // binary operation between all types except structures
      if (!lhs->IsStruct() && lhs->IsIdentical(rhs)) {
        if (lhs->IsArray()) {
          ast.logger()->LogWarning(
              "array comparison always evaluates to a constant value");
        }
        type = MakePrimType(Type::Int32, true);
      }
      break;
    }
    case Op::Assign: {
      // binary operation between all types
      if (lhs->CanAccept(rhs)) type = lhs;
      break;
    }
    case Op::AssAdd:
    case Op::AssSub: {
      // pointer operation
      if (lhs->IsPointer() && !lhs->IsRightValue() && !lhs->IsConst() &&
          rhs->IsInteger()) {
        type = lhs;
        break;
      }
      // fall through
    }
    case Op::AssMul:
    case Op::AssSDiv:
    case Op::AssSRem:
    case Op::AssAnd:
    case Op::AssOr:
    case Op::AssXor:
    case Op::AssShl:
    case Op::AssAShr: {
      // int binary operation
      if (lhs->IsInteger() && lhs->CanAccept(rhs)) type = lhs;
      break;
    }
    default:
      assert(false);
  }

  // update operator type
  auto originOp = ast.op();
  if (originOp == Operator::SDiv    || originOp == Operator::SRem    ||
      originOp == Operator::AShr    || originOp == Operator::AssAShr ||
      originOp == Operator::SLess   || originOp == Operator::SGreat  ||
      originOp == Operator::AssSDiv || originOp == Operator::AssSRem ||
      originOp == Operator::SLessEq || originOp == Operator::SGreatEq) {
    // TODO: here
    if (lhs->IsUnsigned() || rhs->IsUnsigned()) {
      Operator op = originOp;
      switch (originOp) {
        case Operator::SDiv:     op = Operator::UDiv;     break;
        case Operator::SRem:     op = Operator::URem;     break;
        case Operator::SLess:    op = Operator::ULess;    break;
        case Operator::SGreat:   op = Operator::UGreat;   break;
        case Operator::AssSDiv:  op = Operator::AssUDiv;  break;
        case Operator::AssSRem:  op = Operator::AssURem;  break;
        case Operator::SLessEq:  op = Operator::ULessEq;  break;
        case Operator::SGreatEq: op = Operator::UGreatEq; break;
        case Operator::AShr: {
          if (lhs->IsUnsigned()) op = Operator::LShr;
          break;
        }
        case Operator::AssAShr: {
          if (lhs->IsUnsigned()) op = Operator::AssLShr;
          break;
        }
        default: break;
      }
      ast.set_op(op);
    }
  }

  // check return type
  if (!type) return LogError(ast.logger(), "invalid binary operation");
  if (!BinaryStmt::IsOperatorAssign(ast.op()) && !type->IsRightValue()) {
    type = type->GetValueType(true);
  }
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(CastStmt &ast) {
  auto expr = ast.expr()->SemaAnalyze(*this);
  auto type = ast.type()->SemaAnalyze(*this);
  if (!expr || !type) return nullptr;
  // check if cast is valid
  if (!expr->CanCastTo(type)) {
    return LogError(ast.logger(), "invalid type casting");
  }
  return ast.set_ast_type(type->GetValueType(true));
}

TypePtr Analyzer::AnalyzeOn(UnaryStmt &ast) {
  using Op = front::Operator;
  // get operand
  auto opr = ast.opr()->SemaAnalyze(*this);
  if (!opr || opr->IsVoid()) {
    return LogError(ast.opr()->logger(), "invalid operand");
  }
  // handle by operator
  TypePtr type;
  switch (ast.op()) {
    case Op::Pos:
    case Op::Neg:
    case Op::Not:
    case Op::LNot: {
      if (opr->IsInteger()) type = opr;
      break;
    }
    case Op::Deref: {
      if (opr->IsPointer() || opr->IsArray()) type = opr->GetDerefedType();
      break;
    }
    case Op::Addr: {
      if (!opr->IsRightValue()) type = MakePointer(opr);
      break;
    }
    case Op::SizeOf: {
      type = MakePrimType(Type::UInt32, true);
      break;
    }
    default:
      assert(false);
  }
  // check return type
  if (!type) return LogError(ast.logger(), "invalid unary operator");
  if (ast.op() != Op::Deref && !type->IsRightValue()) {
    type = type->GetValueType(true);
  }
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(IndexAST &ast) {
  // get expression
  auto expr = ast.expr()->SemaAnalyze(*this);
  if (!expr || !(expr->IsPointer() || expr->IsArray())) {
    return LogError(ast.expr()->logger(),
                    "expression is not subscriptable");
  }
  // get type of index
  auto index = ast.index()->SemaAnalyze(*this);
  if (!index || !index->IsInteger()) {
    return LogError(ast.index()->logger(), "invalid index");
  }
  // get return type
  auto type = expr->GetDerefedType();
  if (expr->IsArray()) {
    if (auto val = ast.index()->Eval(eval_)) {
      // check if out of bounds
      if (*val >= expr->GetLength()) {
        ast.index()->logger()->LogWarning("subscript out of bounds");
      }
    }
  }
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(CallStmt &ast) {
  // get expression
  auto expr = ast.expr()->SemaAnalyze(*this);
  if (!expr) return nullptr;
  if (!expr || !expr->IsFunction()) {
    return LogError(ast.expr()->logger(), "calling a non-function");
  }
  // get arguments
  TypePtrList args;
  for (const auto &i : ast.args()) {
    auto arg = i->SemaAnalyze(*this);
    if (!arg) return nullptr;
    args.push_back(std::move(arg));
  }
  // check return type
  auto ret = expr->GetReturnType(args);
  if (!ret) return LogError(ast.logger(), "invalid function call");
  return ast.set_ast_type(ret->GetValueType(true));
}

TypePtr Analyzer::AnalyzeOn(AccessAST &ast) {
  // get expression
  auto expr = ast.expr()->SemaAnalyze(*this);
  if (!expr) return nullptr;
  // get dereferenced type
  if (ast.is_arrow()) {
    // check if is pointer
    if (!expr->IsPointer()) {
      return LogError(ast.expr()->logger(), "expression is not a pointer");
    }
    expr = expr->GetDerefedType();
  }
  // check if is valid
  if (!expr->IsStruct()) {
    return LogError(ast.expr()->logger(), "structure type required");
  }
  auto type = expr->GetElem(ast.id());
  if (!type) return LogError(ast.logger(), "member not found", ast.id());
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(IntAST &ast) {
  // make right value 'int32' type
  return ast.set_ast_type(MakePrimType(Type::Int32, true));
}

TypePtr Analyzer::AnalyzeOn(CharAST &ast) {
  // make right value 'int8' type
  return ast.set_ast_type(MakePrimType(Type::Int8, true));
}

TypePtr Analyzer::AnalyzeOn(StringAST &ast) {
  // make right value 'const int8*' type
  auto type = MakePrimType(Type::Int8, true);
  type = std::make_shared<ConstType>(std::move(type));
  return ast.set_ast_type(MakePointer(type));
}

TypePtr Analyzer::AnalyzeOn(VariableAST &ast) {
  // query from environment
  auto type = symbols_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "undefined symbol", ast.id());
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(PrimTypeAST &ast) {
  auto type = MakePrimType(ast.type(), false);
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(UserTypeAST &ast) {
  // query from environment
  auto type = aliases_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "type undefined", ast.id());
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(StructTypeAST &ast) {
  // query from environment
  auto type = structs_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "type undefined", ast.id());
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(EnumTypeAST &ast) {
  // query from environment
  auto type = enums_->GetItem(ast.id());
  if (!type) return LogError(ast.logger(), "type undefined", ast.id());
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(ConstTypeAST &ast) {
  // get base type
  auto base = ast.base()->SemaAnalyze(*this);
  if (!base) return nullptr;
  // make const type
  auto type = std::make_shared<ConstType>(std::move(base));
  return ast.set_ast_type(type);
}

TypePtr Analyzer::AnalyzeOn(PointerTypeAST &ast) {
  // get base type
  auto type = ast.base()->SemaAnalyze(*this);
  if (!type) return nullptr;
  // make pointer type
  for (std::size_t i = 0; i < ast.depth(); ++i) {
    type = MakePointer(type, false);
  }
  return ast.set_ast_type(type);
}
