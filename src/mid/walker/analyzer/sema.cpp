#include "define/ast.h"

#include "mid/walker/analyzer/eval.h"
#include "mid/walker/analyzer/analyzer.h"

using namespace lava::mid;
using namespace lava::define;

TypePtr TranslationUnitDecl::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr VariableDecl::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr VariableDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr InitListAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr ProtoTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr FunctionDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr FuncParamAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr EnumDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr TypeAliasAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructElemAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructElemDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr EnumElemAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr CompoundStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IfElseStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr WhileStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr ControlAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr BinaryStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr CastStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr UnaryStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IndexAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr CallStmt::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr AccessAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr IntAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr CharAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StringAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr VariableAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr PrimTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr StructTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr EnumTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr ConstTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr PointerTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}

TypePtr UserTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.AnalyzeOn(*this);
}


std::optional<std::uint32_t> TranslationUnitDecl::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> VariableDecl::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> VariableDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> InitListAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> ProtoTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> FunctionDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> FuncParamAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> EnumDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> TypeAliasAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructElemAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructElemDefAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> EnumElemAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> CompoundStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IfElseStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> WhileStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> ControlAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> BinaryStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> CastStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> UnaryStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IndexAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> CallStmt::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> AccessAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> IntAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> CharAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StringAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> VariableAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> PrimTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> StructTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> EnumTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> ConstTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> PointerTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}

std::optional<std::uint32_t> UserTypeAST::Eval(Evaluator &eval) {
  return eval.EvalOn(*this);
}
