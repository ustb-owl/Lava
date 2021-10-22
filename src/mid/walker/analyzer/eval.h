#ifndef LAVA_MID_EVAL_H_
#define LAVA_MID_EVAL_H_

#include <optional>
#include <string>
#include <cstdint>

#include "define/ast.h"
#include "lib/guard.h"
#include "lib/nested.h"
#include "mid/visitor/visitor.h"

namespace lava::mid {

// perform compile time evaluation
// NOTE: this evaluator will only try to eval all 'const' definitions
class Evaluator : public Visitor<std::optional<std::uint32_t>> {
 public:
  Evaluator() { Reset(); }

  // reset internal status
  void Reset() {
    values_ =
        xstl::MakeNestedMap<std::string, std::optional<std::uint32_t>>();
  }

  std::optional<std::uint32_t> visit(TranslationUnitDecl  *) override;
  std::optional<std::uint32_t> visit(VariableDecl         *) override;
  std::optional<std::uint32_t> visit(VariableDefAST       *) override;
  std::optional<std::uint32_t> visit(InitListAST          *) override;
  std::optional<std::uint32_t> visit(ProtoTypeAST         *) override;
  std::optional<std::uint32_t> visit(FunctionDefAST       *) override;
  std::optional<std::uint32_t> visit(FuncParamAST         *) override;
  std::optional<std::uint32_t> visit(StructDefAST         *) override;
  std::optional<std::uint32_t> visit(EnumDefAST           *) override;
  std::optional<std::uint32_t> visit(TypeAliasAST         *) override;
  std::optional<std::uint32_t> visit(StructElemAST        *) override;
  std::optional<std::uint32_t> visit(StructElemDefAST     *) override;
  std::optional<std::uint32_t> visit(EnumElemAST          *) override;
  std::optional<std::uint32_t> visit(CompoundStmt         *) override;
  std::optional<std::uint32_t> visit(IfElseStmt           *) override;
  std::optional<std::uint32_t> visit(WhileStmt            *) override;
  std::optional<std::uint32_t> visit(ControlAST           *) override;
  std::optional<std::uint32_t> visit(BinaryStmt           *) override;
  std::optional<std::uint32_t> visit(CastStmt             *) override;
  std::optional<std::uint32_t> visit(UnaryStmt            *) override;
  std::optional<std::uint32_t> visit(IndexAST             *) override;
  std::optional<std::uint32_t> visit(CallStmt             *) override;
  std::optional<std::uint32_t> visit(AccessAST            *) override;
  std::optional<std::uint32_t> visit(IntAST               *) override;
  std::optional<std::uint32_t> visit(CharAST              *) override;
  std::optional<std::uint32_t> visit(StringAST            *) override;
  std::optional<std::uint32_t> visit(VariableAST          *) override;
  std::optional<std::uint32_t> visit(PrimTypeAST          *) override;
  std::optional<std::uint32_t> visit(StructTypeAST        *) override;
  std::optional<std::uint32_t> visit(EnumTypeAST          *) override;
  std::optional<std::uint32_t> visit(ConstTypeAST         *) override;
  std::optional<std::uint32_t> visit(PointerTypeAST       *) override;
  std::optional<std::uint32_t> visit(UserTypeAST          *) override;

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
