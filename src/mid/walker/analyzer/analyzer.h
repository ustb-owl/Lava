#ifndef MIMIC_MID_ANALYZER_H_
#define MIMIC_MID_ANALYZER_H_

#include <string>
#include <string_view>
#include <unordered_set>
#include <stack>
#include <unordered_map>
#include <cstddef>

#include "lib/guard.h"
#include "lib/nested.h"
#include "define/ast.h"
#include "mid/walker/analyzer/eval.h"

namespace lava::mid {

// perform semantic analysis
class Analyzer {
 public:
  Analyzer(Evaluator &eval) : eval_(eval) { Reset(); }

  void Reset();

  define::TypePtr AnalyzeOn(define::TranslationUnitDecl &ast);
  define::TypePtr AnalyzeOn(define::VariableDecl &ast);
  define::TypePtr AnalyzeOn(define::VariableDefAST &ast);
  define::TypePtr AnalyzeOn(define::InitListAST &ast);
  define::TypePtr AnalyzeOn(define::ProtoTypeAST &ast);
  define::TypePtr AnalyzeOn(define::FunctionDefAST &ast);
  define::TypePtr AnalyzeOn(define::FuncParamAST &ast);
  define::TypePtr AnalyzeOn(define::StructDefAST &ast);
  define::TypePtr AnalyzeOn(define::EnumDefAST &ast);
  define::TypePtr AnalyzeOn(define::TypeAliasAST &ast);
  define::TypePtr AnalyzeOn(define::StructElemAST &ast);
  define::TypePtr AnalyzeOn(define::StructElemDefAST &ast);
  define::TypePtr AnalyzeOn(define::EnumElemAST &ast);
  define::TypePtr AnalyzeOn(define::CompoundStmt &ast);
  define::TypePtr AnalyzeOn(define::IfElseStmt &ast);
  define::TypePtr AnalyzeOn(define::WhileStmt &ast);
  define::TypePtr AnalyzeOn(define::ControlAST &ast);
  define::TypePtr AnalyzeOn(define::BinaryStmt &ast);
  define::TypePtr AnalyzeOn(define::CastStmt &ast);
  define::TypePtr AnalyzeOn(define::UnaryStmt &ast);
  define::TypePtr AnalyzeOn(define::IndexAST &ast);
  define::TypePtr AnalyzeOn(define::CallStmt &ast);
  define::TypePtr AnalyzeOn(define::AccessAST &ast);
  define::TypePtr AnalyzeOn(define::IntAST &ast);
  define::TypePtr AnalyzeOn(define::CharAST &ast);
  define::TypePtr AnalyzeOn(define::StringAST &ast);
  define::TypePtr AnalyzeOn(define::VariableAST &ast);
  define::TypePtr AnalyzeOn(define::PrimTypeAST &ast);
  define::TypePtr AnalyzeOn(define::StructTypeAST &ast);
  define::TypePtr AnalyzeOn(define::EnumTypeAST &ast);
  define::TypePtr AnalyzeOn(define::ConstTypeAST &ast);
  define::TypePtr AnalyzeOn(define::PointerTypeAST &ast);
  define::TypePtr AnalyzeOn(define::UserTypeAST &ast);

 private:
  // pointer of symbol table (environment)
  using EnvPtr = xstl::NestedMapPtr<std::string, define::TypePtr>;

  // function information
  struct FuncInfo {
    define::TypePtr type;
    bool is_decl;
  };

  // switch to new environment
  xstl::Guard NewEnv();
  // handle array type
  define::TypePtr HandleArray(define::TypePtr base,
                              const define::ASTPtrList &arr_lens,
                              std::string_view id, bool is_param);

  // base type of all enumerators
  static define::TypePtr enum_base_;

  // evaluator
  Evaluator &eval_;
  // symbol table, aliases, structs, enums
  EnvPtr symbols_, aliases_, structs_, enums_;
  // used when analyzing var/const declarations
  define::TypePtr var_type_;
  // used when analyzing initializer list
  std::stack<define::TypePtr> final_types_;
  // used when analyzing function related stuffs
  bool in_func_;
  define::TypePtr cur_ret_;
  std::unordered_map<std::string_view, FuncInfo> funcs_;
  // used when analyzing structs
  std::string_view last_struct_name_;
  define::TypePairList struct_elems_;
  std::unordered_set<std::string_view> struct_elem_names_;
  define::TypePtr struct_elem_base_;
  // used when analyzing while loops
  std::size_t in_loop_;
};

}  // namespace lava::mid

#endif  // MIMIC_MID_ANALYZER_H_
