#ifndef LAVA_DEFINE_AST_H_
#define LAVA_DEFINE_AST_H_

#include <ostream>
#include <optional>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>
#include <cstddef>

#include "front/token.h"
#include "define/type.h"
#include "front/logger.h"
#include "mid/ir/usedef/value.h"


// forward declarations for visitor pattern
namespace lava::mid {
class Analyzer;
class Evaluator;
class IRBuilder;
}  // namespace lava::mid

namespace lava::define {

class BaseAST;
using ASTPtr = std::unique_ptr<BaseAST>;
using ASTPtrList = std::vector<ASTPtr>;

// definition of base class of all ASTs
class BaseAST {
 public:
  virtual ~BaseAST() = default;

  // return true if current AST is a literal value
  virtual bool IsLiteral() const = 0;
  // return true if current AST is a initializer list
  virtual bool IsInitList() const = 0;

  // dump the content of AST (XML format) to output stream
  virtual void Dump(std::ostream &os) const = 0;
  // run sematic analysis on current AST
  virtual TypePtr SemaAnalyze(mid::Analyzer &ana) = 0;
  // evaluate AST (if possible)
  virtual std::optional<std::uint32_t> Eval(mid::Evaluator &eval) = 0;
  // generate IR by current AST
  virtual mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) = 0;
  // merge two ast
  virtual void Merge(ASTPtrList &) {};

  // setters
  void set_logger(const front::LoggerPtr &logger) { logger_ = logger; }
  const TypePtr &set_ast_type(const TypePtr &ast_type) {
    return ast_type_ = ast_type;
  }

  // getters
  const front::LoggerPtr &logger() const {
    return logger_;
  }

  const TypePtr &ast_type() const { return ast_type_; }
  virtual std::string ArgName() const {
    DBG_ASSERT(0, "can't reach here");
    return "";
  }

private:
  front::LoggerPtr logger_;
  TypePtr ast_type_;
};


class Stmt : public BaseAST {
};

class Decl : public BaseAST {
};

class TranslationUnitDecl : public Decl {
private:
  ASTPtrList decls_;

public:
  explicit TranslationUnitDecl(ASTPtrList _decls)
    : decls_(std::move(_decls)) {}

  bool IsLiteral() const override { return false; };
  bool IsInitList() const override { return false; };


  ASTPtrList &decls() { return decls_; }

  void Merge(ASTPtrList &decls) override {
    decls.insert(decls.end(),
                 std::make_move_iterator(decls_.begin()),
                 std::make_move_iterator(decls_.end()));
    decls_ = std::move(decls);
  }

  void Dump(std::ostream &os) const override;

  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;
};


// variable/constant declaration
class VariableDecl : public BaseAST {
 public:
  VariableDecl(ASTPtr type, ASTPtrList defs)
      : type_(std::move(type)), defs_(std::move(defs)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const ASTPtrList &defs() const { return defs_; }

 private:
  ASTPtr type_;
  ASTPtrList defs_;
};

// variable/constant definition
class VariableDefAST : public BaseAST {
 public:
  VariableDefAST(const std::string &id, ASTPtrList arr_lens, ASTPtr init)
      : id_(id), arr_lens_(std::move(arr_lens)), init_(std::move(init)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  bool hasInit() const { return init_ != nullptr; }

  // setters
  void set_init(ASTPtr init) { init_ = std::move(init); }

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &arr_lens() const { return arr_lens_; }
  const ASTPtr &init() const { return init_; }

 private:
  std::string id_;
  ASTPtrList arr_lens_;
  ASTPtr init_;
};

// initializer list (for array initialization)
class InitListAST : public BaseAST {
 public:
  InitListAST(ASTPtrList exprs) : exprs_(std::move(exprs)) {}

  bool IsLiteral() const override {
    for (const auto &i : exprs_) {
      if (!i->IsLiteral()) return false;
    }
    return true;
  }
  bool IsInitList() const override { return true; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setters
  void set_exprs(ASTPtrList exprs) { exprs_ = std::move(exprs); }
  void set_expr(std::size_t i, ASTPtr expr) {
    exprs_[i] = std::move(expr);
  }

  // getters
  // NOTE: non-const getter
  ASTPtrList &exprs() { return exprs_; }

 private:
  ASTPtrList exprs_;
};

// function declaration
class ProtoTypeAST : public BaseAST {
 public:
  ProtoTypeAST(ASTPtr type, const std::string &id, ASTPtrList params)
      : type_(std::move(type)), id_(id), params_(std::move(params)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const std::string &id() const { return id_; }
  const ASTPtrList &params() const { return params_; }

 private:
  ASTPtr type_;
  std::string id_;
  ASTPtrList params_;
};

// function definition
class FunctionDefAST : public BaseAST {
 public:
  FunctionDefAST(ASTPtr header, ASTPtr body)
      : header_(std::move(header)), body_(std::move(body)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &header() const { return header_; }
  const ASTPtr &body() const { return body_; }

 private:
  ASTPtr header_, body_;
};

// function parameter
// NOTE: if parameter is an array, 'arr_lens_' must not be empty
//       but it's first element can be 'nullptr' (e.g. int arg[])
class FuncParamAST : public BaseAST {
 public:
  FuncParamAST(ASTPtr type, const std::string &id, ASTPtrList arr_lens)
      : type_(std::move(type)), id_(id), arr_lens_(std::move(arr_lens)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &type() const { return type_; }
  std::string ArgName() const override { return id_; }
  const ASTPtrList &arr_lens() const { return arr_lens_; }

 private:
  ASTPtr type_;
  std::string id_;
  ASTPtrList arr_lens_;
};

// structure definition
class StructDefAST : public BaseAST {
 public:
  StructDefAST(const std::string &id, ASTPtrList elems)
      : id_(id), elems_(std::move(elems)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &elems() const { return elems_; }

 private:
  std::string id_;
  ASTPtrList elems_;
};

// enumeration definition
// NOTE: property 'id' can be empty
class EnumDefAST : public BaseAST {
 public:
  EnumDefAST(const std::string &id, ASTPtrList elems)
      : id_(id), elems_(std::move(elems)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &elems() const { return elems_; }

 private:
  std::string id_;
  ASTPtrList elems_;
};

// type alias
class TypeAliasAST : public BaseAST {
 public:
  TypeAliasAST(ASTPtr type, const std::string &id)
      : type_(std::move(type)), id_(id) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const std::string &id() const { return id_; }

 private:
  ASTPtr type_;
  std::string id_;
};

// element of structure
class StructElemAST : public BaseAST {
 public:
  StructElemAST(ASTPtr type, ASTPtrList defs)
      : type_(std::move(type)), defs_(std::move(defs)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &type() const { return type_; }
  const ASTPtrList &defs() const { return defs_; }

 private:
  ASTPtr type_;
  ASTPtrList defs_;
};

// element definition of structure
class StructElemDefAST : public BaseAST {
 public:
  StructElemDefAST(const std::string &id, ASTPtrList arr_lens)
      : id_(id), arr_lens_(std::move(arr_lens)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }
  const ASTPtrList &arr_lens() const { return arr_lens_; }

 private:
  std::string id_;
  ASTPtrList arr_lens_;
};

// element of enumeration
class EnumElemAST : public BaseAST {
 public:
  EnumElemAST(const std::string &id, ASTPtr expr)
      : id_(id), expr_(std::move(expr)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setters
  void set_expr(ASTPtr expr) { expr_ = std::move(expr); }

  // getters
  const std::string &id() const { return id_; }
  const ASTPtr &expr() const { return expr_; }

 private:
  std::string id_;
  ASTPtr expr_;
};

// statement block
class CompoundStmt : public BaseAST {
 public:
  CompoundStmt(ASTPtrList stmts) : stmts_(std::move(stmts)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtrList &stmts() const { return stmts_; }

 private:
  ASTPtrList stmts_;
};

// if-else statement
class IfElseStmt : public BaseAST {
 public:
  IfElseStmt(ASTPtr cond, ASTPtr then, ASTPtr else_then)
      : cond_(std::move(cond)), then_(std::move(then)),
        else_then_(std::move(else_then)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  bool hasElse() const { return else_then_ != nullptr; }

  // setter
  void set_cond(ASTPtr cond) { cond_ = std::move(cond); }

  // getters
  const ASTPtr &cond() const { return cond_; }
  const ASTPtr &then() const { return then_; }
  const ASTPtr &else_then() const { return else_then_; }

 private:
  ASTPtr cond_, then_, else_then_;
};

// while statement
class WhileStmt : public BaseAST {
 public:
  WhileStmt(ASTPtr cond, ASTPtr body)
      : cond_(std::move(cond)), body_(std::move(body)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setter
  void set_cond(ASTPtr cond) { cond_ = std::move(cond); }

  // getters
  const ASTPtr &cond() const { return cond_; }
  const ASTPtr &body() const { return body_; }

 private:
  ASTPtr cond_, body_;
};

// control statement
class ControlAST : public BaseAST {
 public:
  enum class Type { Break, Continue, Return };

  ControlAST(Type type, ASTPtr expr)
      : type_(type), expr_(std::move(expr)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setter
  void set_expr(ASTPtr expr) { expr_ = std::move(expr); }

  // getters
  Type type() const { return type_; }
  const ASTPtr &expr() const { return expr_; }

 private:
  Type type_;
  ASTPtr expr_;
};

// binary expression
class BinaryStmt : public BaseAST {
 public:
  using Operator = lava::front::Operator;
  BinaryStmt(Operator op, ASTPtr lhs, ASTPtr rhs)
      : op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  // check if is an assignment operator
  static bool IsOperatorAssign(Operator op) {
    return static_cast<int>(op) >= static_cast<int>(Operator::Assign);
  }
  // get de-assigned operator ('+=' -> '+', '-=' -> '-', ...)
  static Operator GetDeAssignedOp(Operator op) {
    switch (op) {
      case Operator::AssAdd: return Operator::Add;
      case Operator::AssSub: return Operator::Sub;
      case Operator::AssMul: return Operator::Mul;
      case Operator::AssSDiv: return Operator::SDiv;
      case Operator::AssSRem: return Operator::SRem;
      case Operator::AssAnd: return Operator::And;
      case Operator::AssOr:  return Operator::Or;
      case Operator::AssXor: return Operator::Xor;
      case Operator::AssShl: return Operator::Shl;
      case Operator::AssLShr: return Operator::LShr;
      default: assert(false); return Operator::Assign;
    }
  }

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setters
  void set_op(Operator op) { op_ = op; }
  void set_lhs(ASTPtr lhs) { lhs_ = std::move(lhs); }
  void set_rhs(ASTPtr rhs) { rhs_ = std::move(rhs); }

  // getters
  Operator op() const { return op_; }
  const ASTPtr &lhs() const { return lhs_; }
  const ASTPtr &rhs() const { return rhs_; }

 private:
  Operator op_;
  ASTPtr lhs_, rhs_;
};

// type casting expression
class CastStmt : public BaseAST {
 public:
  CastStmt(ASTPtr type, ASTPtr expr)
      : type_(std::move(type)), expr_(std::move(expr)) {}

  bool IsLiteral() const override { return expr_->IsLiteral(); }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setters
  void set_expr(ASTPtr expr) { expr_ = std::move(expr); }

  // getters
  const ASTPtr &type() const { return type_; }
  const ASTPtr &expr() const { return expr_; }

 private:
  ASTPtr type_, expr_;
};

// unary expression
class UnaryStmt : public BaseAST {
 public:
  using Operator = front::Operator;
  UnaryStmt(Operator op, ASTPtr opr)
      : op_(op), opr_(std::move(opr)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setters
  void set_opr(ASTPtr opr) { opr_ = std::move(opr); }

  // getters
  Operator op() const { return op_; }
  const ASTPtr &opr() const { return opr_; }

 private:
  Operator op_;
  ASTPtr opr_;
};

// indexing
class IndexAST : public BaseAST {
 public:
  IndexAST(ASTPtr expr, ASTPtr index)
      : expr_(std::move(expr)), index_(std::move(index)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setter
  void set_index(ASTPtr index) { index_ = std::move(index); }

  // getters
  const ASTPtr &expr() const { return expr_; }
  const ASTPtr &index() const { return index_; }

 private:
  ASTPtr expr_, index_;
};

// function call
class CallStmt : public BaseAST {
 public:
  CallStmt(ASTPtr expr, ASTPtrList args)
      : expr_(std::move(expr)), args_(std::move(args)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // setters
  void set_arg(std::size_t i, ASTPtr arg) {
    args_[i] = std::move(arg);
  }

  // getters
  const ASTPtr &expr() const { return expr_; }
  const ASTPtrList &args() const { return args_; }

 private:
  ASTPtr expr_;
  ASTPtrList args_;
};

// accessing
// NOTE: property 'is_arrow' can distinguish the accessing opration
//       between '->' (arrow type) and '.' (dot type)
class AccessAST : public BaseAST {
 public:
  AccessAST(bool is_arrow, ASTPtr expr, std::string id)
      : is_arrow_(is_arrow), expr_(std::move(expr)), id_(std::move(id)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  bool is_arrow() const { return is_arrow_; }
  const ASTPtr &expr() const { return expr_; }
  const std::string &id() const { return id_; }

 private:
  bool is_arrow_;
  ASTPtr expr_;
  std::string id_;
};

// integer number literal
class IntAST : public BaseAST {
 public:
  IntAST(std::uint32_t value) : value_(value) {}

  bool IsLiteral() const override { return true; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  std::uint32_t value() const { return value_; }

 private:
  std::uint32_t value_;
};

// character literal
class CharAST : public BaseAST {
 public:
  CharAST(std::uint8_t c) : c_(c) {}

  bool IsLiteral() const override { return true; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  std::uint8_t c() const { return c_; }

 private:
  std::uint8_t c_;
};

// string literal
class StringAST : public BaseAST {
 public:
  StringAST(const std::string &str) : str_(str) {}

  bool IsLiteral() const override { return true; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &str() const { return str_; }

 private:
  std::string str_;
};

// identifier
class VariableAST : public BaseAST {
 public:
  VariableAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

// primitive type
class PrimTypeAST : public BaseAST {
 public:
  PrimTypeAST(Type type) : type_(type) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  Type type() const { return type_; }

 private:
  Type type_;
};

// structure type
class StructTypeAST : public BaseAST {
 public:
  StructTypeAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

// enumeration type
class EnumTypeAST : public BaseAST {
 public:
  EnumTypeAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

// constant type
class ConstTypeAST : public BaseAST {
 public:
  ConstTypeAST(ASTPtr base) : base_(std::move(base)) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &base() const { return base_; }

 private:
  ASTPtr base_;
};

// pointer type
// NOTE: property 'depth' means count of '*' symbol
class PointerTypeAST : public BaseAST {
 public:
  PointerTypeAST(ASTPtr base, std::size_t depth)
      : base_(std::move(base)), depth_(depth) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const ASTPtr &base() const { return base_; }
  std::size_t depth() const { return depth_; }

 private:
  ASTPtr base_;
  std::size_t depth_;
};

// user defined type (type aliases)
class UserTypeAST : public BaseAST {
 public:
  UserTypeAST(const std::string &id) : id_(id) {}

  bool IsLiteral() const override { return false; }
  bool IsInitList() const override { return false; }

  void Dump(std::ostream &os) const override;
  TypePtr SemaAnalyze(mid::Analyzer &ana) override;
  std::optional<std::uint32_t> Eval(mid::Evaluator &eval) override;
  mid::SSAPtr CodeGeneAction(mid::IRBuilder *irbuilder) override;

  // getters
  const std::string &id() const { return id_; }

 private:
  std::string id_;
};

}  // namespace lava::define

#endif  // LAVA_DEFINE_AST_H_
