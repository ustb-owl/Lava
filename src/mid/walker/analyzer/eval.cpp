#include "mid/walker/analyzer/eval.h"

#include <cassert>

using namespace lava::mid;
using namespace lava::define;

namespace {

// create a new integer AST
inline ASTPtr MakeAST(std::uint32_t val, const ASTPtr &ast) {
  auto ret = std::make_unique<IntAST>(val);
  ret->set_logger(ast->logger());
  // check if is right value
  const auto &type = ast->ast_type();
  if (type->IsRightValue()) {
    ret->set_ast_type(type);
  } else {
    ret->set_ast_type(type->GetValueType(true));
  }
  return ret;
}

// cast the specific integer to type
inline std::optional<std::uint32_t> CastToType(std::uint32_t val,
                                               const TypePtr &type) {
  assert(type->IsInteger() || type->IsPointer());
  if (type->IsInteger() || type->IsPointer()) {
    if (type->GetSize() == 1) {
      // i8/u8
      return type->IsUnsigned() ? static_cast<std::uint8_t>(val)
                                : static_cast<std::int8_t>(val);
    }
    else if (type->GetSize() == 32) {
      // i32/u32/ptrs
      return type->IsUnsigned() || type->IsPointer()
                  ? static_cast<std::uint32_t>(val)
                  : static_cast<std::int32_t>(val);
    }
    else {
      assert(false);
      return 0;
    }
  }
  else {
    return {};
  }
}

}  // namespace

xstl::Guard Evaluator::NewEnv() {
  values_ = xstl::MakeNestedMap(values_);
  return xstl::Guard([this] { values_ = values_->outer(); });
}


std::optional<std::uint32_t> Evaluator::visit(TranslationUnitDecl *ast) {
  auto guard = NewEnv();
  for (const auto &i : ast->decls()) i->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(VariableDecl *ast) {
  // evaluate constant integers only
  const auto &type = ast->type()->ast_type();
  is_const_int_ = type->IsConst() && type->IsInteger();
  // evaluate definitions
  for (const auto &i : ast->defs()) i->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(VariableDefAST *ast) {
  // evaluate initial value
  if (!ast->init()) return {};
  auto val = ast->init()->Eval(*this);
  if (!val) return {};
  // perform implicit type casting
  val = CastToType(*val, ast->ast_type());
  // add to environment
  if (is_const_int_) values_->AddItem(ast->id(), val);
  // update AST
  ast->set_init(MakeAST(*val, ast->init()));
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(InitListAST *ast) {
  for (std::size_t i = 0; i < ast->exprs().size(); ++i) {
    if (auto expr = ast->exprs()[i]->Eval(*this)) {
      // update AST
      ast->set_expr(i, MakeAST(*expr, ast->exprs()[i]));
    }
  }
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(ProtoTypeAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(FunctionDefAST *ast) {
  ast->body()->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(FuncParamAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(StructDefAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(EnumDefAST *ast) {
  last_enum_val_ = 0;
  // evaluate all element definitions in enumeration
  for (const auto &i : ast->elems()) i->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(TypeAliasAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(StructElemAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(StructElemDefAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(EnumElemAST *ast) {
  // check if has initial expression
  if (ast->expr()) {
    auto val = ast->expr()->Eval(*this);
    assert(val);
    last_enum_val_ = *val;
    // update AST
    ast->set_expr(MakeAST(*val, ast->expr()));
  }
  // add to environment
  values_->AddItem(ast->id(), last_enum_val_++);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(CompoundStmt *ast) {
  auto guard = NewEnv();
  for (const auto &i : ast->stmts()) i->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(IfElseStmt *ast) {
  // evaluate condition
  auto cond = ast->cond()->Eval(*this);
  if (cond) ast->set_cond(MakeAST(*cond, ast->cond()));
  // evaluate blocks
  ast->then()->Eval(*this);
  if (ast->else_then()) ast->else_then()->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(WhileStmt *ast) {
  // evaluate condition
  auto cond = ast->cond()->Eval(*this);
  if (cond) ast->set_cond(MakeAST(*cond, ast->cond()));
  // evaluate body
  ast->body()->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(ControlAST *ast) {
  // evaluate expression
  if (ast->expr()) {
    auto expr = ast->expr()->Eval(*this);
    if (expr) ast->set_expr(MakeAST(*expr, ast->expr()));
  }
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(BinaryStmt *ast) {
  using Op = front::Operator;
  // evaluate & update rhs
  auto rhs = ast->rhs()->Eval(*this);
  if (rhs) ast->set_rhs(MakeAST(*rhs, ast->rhs()));
  // do not evaluate binary expressions with assign operators
  if (BinaryStmt::IsOperatorAssign(ast->op())) return {};
  // evaluate & update lhs
  auto lhs = ast->lhs()->Eval(*this);
  if (lhs) ast->set_lhs(MakeAST(*lhs, ast->lhs()));
  if (!lhs || !rhs) return {};
  // calculate result
  const auto &type = ast->ast_type();
  auto lv = *CastToType(*lhs, type), rv = *CastToType(*rhs, type);
  auto slv = static_cast<std::int32_t>(lv);
  auto srv = static_cast<std::int32_t>(rv);
  switch (ast->op()) {
    case Op::Add:         return lv + rv;
    case Op::Sub:         return lv - rv;
    case Op::Mul:         return type->IsUnsigned() ? lv * rv : slv * srv;
    case Op::SDiv:        return type->IsUnsigned() ? lv / rv : slv / srv;
    case Op::SRem:        return type->IsUnsigned() ? lv % rv : slv % srv;
    case Op::And:         return lv & rv;
    case Op::Or:          return lv | rv;
    case Op::Xor:         return lv ^ rv;
    case Op::Shl:         return lv << rv;
    case Op::LShr:        return type->IsUnsigned() ? lv >> rv : slv >> srv;
    case Op::LAnd:    return lv && rv;
    case Op::LOr:     return lv || rv;
    case Op::Equal:       return lv == rv;
    case Op::NotEqual:    return lv != rv;
    case Op::SLess:       return type->IsUnsigned() ? lv < rv : slv < srv;
    case Op::SLessEq:     return type->IsUnsigned() ? lv <= rv : slv <= srv;
    case Op::SGreat:      return type->IsUnsigned() ? lv > rv : slv > srv;
    case Op::SGreatEq:    return type->IsUnsigned() ? lv >= rv : slv >= srv;
    default: assert(false); return {};
  }
}

std::optional<std::uint32_t> Evaluator::visit(CastStmt *ast) {
  // evaluate expression
  auto expr = ast->expr()->Eval(*this);
  if (!expr) return {};
  ast->set_expr(MakeAST(*expr, ast->expr()));
  // perform type casting
  return CastToType(*expr, ast->type()->ast_type());
}

std::optional<std::uint32_t> Evaluator::visit(UnaryStmt *ast) {
  using Op = front::Operator;
  // evaluate operand
  auto opr = ast->opr()->Eval(*this);
  if (!opr) return {};
  ast->set_opr(MakeAST(*opr, ast->opr()));
  // calculate result
  auto val = *opr;
  switch (ast->op()) {
    case Op::Pos: return +val;
    case Op::Neg: return -val;
    case Op::Not: return ~val;
    case Op::LNot: return !val;
    case Op::Deref: return {};
    case Op::Addr: return {};
    case Op::SizeOf: return ast->opr()->ast_type()->GetSize();
    default: assert(false); return {};
  }
}

std::optional<std::uint32_t> Evaluator::visit(IndexAST *ast) {
  // evaluate expression
  ast->expr()->Eval(*this);
  // evaluate & update index
  auto val = ast->index()->Eval(*this);
  if (val) ast->set_index(MakeAST(*val, ast->index()));
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(CallStmt *ast) {
  // evaluate expression
  ast->expr()->Eval(*this);
  // evaluate & update arguments
  for (std::size_t i = 0; i < ast->args().size(); ++i) {
    auto val = ast->args()[i]->Eval(*this);
    if (val) ast->set_arg(i, MakeAST(*val, ast->args()[i]));
  }
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(AccessAST *ast) {
  // evaluate expression
  ast->expr()->Eval(*this);
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(IntAST *ast) {
  return ast->value();
}

std::optional<std::uint32_t> Evaluator::visit(CharAST *ast) {
  return ast->c();
}

std::optional<std::uint32_t> Evaluator::visit(StringAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(VariableAST *ast) {
  // query the environment and return the requested value if possible
  return values_->GetItem(ast->id());
}

std::optional<std::uint32_t> Evaluator::visit(PrimTypeAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(StructTypeAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(EnumTypeAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(ConstTypeAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(PointerTypeAST *ast) {
  return {};
}

std::optional<std::uint32_t> Evaluator::visit(UserTypeAST *ast) {
  return {};
}
