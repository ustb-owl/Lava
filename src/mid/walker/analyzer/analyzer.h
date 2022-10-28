#ifndef LAVA_MID_ANALYZER_H_
#define LAVA_MID_ANALYZER_H_

#include <string>
#include <string_view>
#include <unordered_set>
#include <stack>
#include <unordered_map>
#include <cstddef>

#include "lib/guard.h"
#include "lib/nested.h"
#include "define/ast.h"
#include "mid/visitor/visitor.h"
#include "mid/walker/analyzer/eval.h"

namespace lava::mid {

// perform semantic analysis
class Analyzer : public Visitor<TypePtr> {
public:
  Analyzer(Evaluator &eval) : eval_(eval) { Reset(); }

  void Reset();

  TypePtr visit(TranslationUnitDecl *) override;
  TypePtr visit(VariableDecl        *) override;
  TypePtr visit(VariableDefAST      *) override;
  TypePtr visit(InitListAST         *) override;
  TypePtr visit(ProtoTypeAST        *) override;
  TypePtr visit(FunctionDefAST      *) override;
  TypePtr visit(FuncParamAST        *) override;
  TypePtr visit(StructDefAST        *) override;
  TypePtr visit(EnumDefAST          *) override;
  TypePtr visit(TypeAliasAST        *) override;
  TypePtr visit(StructElemAST       *) override;
  TypePtr visit(StructElemDefAST    *) override;
  TypePtr visit(EnumElemAST         *) override;
  TypePtr visit(CompoundStmt        *) override;
  TypePtr visit(IfElseStmt          *) override;
  TypePtr visit(WhileStmt           *) override;
  TypePtr visit(ControlAST          *) override;
  TypePtr visit(BinaryStmt          *) override;
  TypePtr visit(CastStmt            *) override;
  TypePtr visit(UnaryStmt           *) override;
  TypePtr visit(IndexAST            *) override;
  TypePtr visit(CallStmt            *) override;
  TypePtr visit(AccessAST           *) override;
  TypePtr visit(IntAST              *) override;
  TypePtr visit(CharAST             *) override;
  TypePtr visit(StringAST           *) override;
  TypePtr visit(VariableAST         *) override;
  TypePtr visit(PrimTypeAST         *) override;
  TypePtr visit(StructTypeAST       *) override;
  TypePtr visit(EnumTypeAST         *) override;
  TypePtr visit(ConstTypeAST        *) override;
  TypePtr visit(PointerTypeAST      *) override;
  TypePtr visit(UserTypeAST         *) override;

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

  define::ASTPtrList GetLinearInitList(define::ASTPtrList &result);
  define::ASTPtrList ListToMatrix(std::deque<std::size_t> dim,
                                  define::ASTPtrList &initList, bool is_top = false);

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
  // array length list
  std::deque<std::size_t> array_lens_;
  // is the top dimension of init list
  bool is_top_dim_ = false;
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

#endif  // LAVA_MID_ANALYZER_H_
