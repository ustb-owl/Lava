#ifndef LAVA_MID_EVAL_H_
#define LAVA_MID_EVAL_H_

#include <optional>
#include <string>
#include <cstdint>

#include "define/ast.h"

#include "lib/guard.h"
#include "lib/nested.h"

namespace lava::mid {

// perform compile time evaluation
// NOTE: this evaluator will only try to eval all 'const' definitions
class Evaluator {
 public:
  Evaluator() { Reset(); }

  // reset internal status
  void Reset() {
    values_ =
        xstl::MakeNestedMap<std::string, std::optional<std::uint32_t>>();
  }

  std::optional<std::uint32_t> EvalOn(define::TranslationUnitDecl &ast);
  std::optional<std::uint32_t> EvalOn(define::VariableDecl &ast);
  std::optional<std::uint32_t> EvalOn(define::VariableDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::InitListAST &ast);
  std::optional<std::uint32_t> EvalOn(define::ProtoTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::FunctionDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::FuncParamAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::EnumDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::TypeAliasAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructElemAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructElemDefAST &ast);
  std::optional<std::uint32_t> EvalOn(define::EnumElemAST &ast);
  std::optional<std::uint32_t> EvalOn(define::CompoundStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::IfElseStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::WhileStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::ControlAST &ast);
  std::optional<std::uint32_t> EvalOn(define::BinaryStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::CastStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::UnaryStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::IndexAST &ast);
  std::optional<std::uint32_t> EvalOn(define::CallStmt &ast);
  std::optional<std::uint32_t> EvalOn(define::AccessAST &ast);
  std::optional<std::uint32_t> EvalOn(define::IntAST &ast);
  std::optional<std::uint32_t> EvalOn(define::CharAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StringAST &ast);
  std::optional<std::uint32_t> EvalOn(define::VariableAST &ast);
  std::optional<std::uint32_t> EvalOn(define::PrimTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::StructTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::EnumTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::ConstTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::PointerTypeAST &ast);
  std::optional<std::uint32_t> EvalOn(define::UserTypeAST &ast);

 private:
  // definition of environment that storing evaluated values
  using EvalEnvPtr =
      xstl::NestedMapPtr<std::string, std::optional<std::uint32_t>>;

  // switch to new environment
  xstl::Guard NewEnv();
  // add value to environment
  void AddValue(const std::string &id, std::uint32_t val);

  // evaluated values
  EvalEnvPtr values_;
  // used when evaluating var/const declarations
  bool is_const_int_;
  // enumeration related stuffs
  std::int32_t last_enum_val_;
};

}  // namespace lava::mid

#endif  // LAVA_MID_EVAL_H_
