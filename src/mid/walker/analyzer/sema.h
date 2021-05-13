#ifndef LAVA__SEMA_H
#define LAVA__SEMA_H

#include <stack>

#include "lib/guard.h"
#include "lib/nested.h"
#include "define/type.h"
#include "front/logger.h"
#include "mid/visitor/visitor.h"

using namespace lava::front;

namespace lava::mid::analyzer {

class SemAnalyzer : Visitor<TypePtr> {
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
  static define::TypePtr _enum_base;

  // evaluator
  Evaluator &_eval;

  EnvPtr                                         _symbols;  // symbol table
  EnvPtr                                         _aliases;  // aliases
  EnvPtr                                         _structs;  // structs
  EnvPtr                                         _enums;    // enums

  // used when analyzing structs
  std::string_view                               _last_struct_name;
  define::TypePairList                           _struct_elems;
  std::unordered_set<std::string_view>           _struct_elem_names;
  define::TypePtr                                _struct_elem_base;

  // used when analyzing while loops
  std::size_t                                    _in_loop;

  // used when analyzing function related stuffs
  bool                                           _in_func;
  std::unordered_map<std::string_view, FuncInfo> _funcs;

  define::TypePtr             _var_type;    // used when analyzing var/const declarations
  std::stack<define::TypePtr> _final_types; // used when analyzing initializer list
  define::TypePtr             _cur_ret;

public:
  explicit SemAnalyzer(ASTPtr &ast) : _rootNode(ast) {}

  TypePtr visit(IntAST              *) override;
  TypePtr visit(CharAST             *) override;
  TypePtr visit(StringAST           *) override;
  TypePtr visit(VariableAST         *) override;
  TypePtr visit(VariableDecl        *) override;
  TypePtr visit(VariableDefAST      *) override;
  TypePtr visit(InitListAST         *) override; // new
  TypePtr visit(StructDefAST        *) override; // new
  TypePtr visit(EnumDefAST          *) override;
  TypePtr visit(TypeAliasAST        *) override; // new
  TypePtr visit(StructElemAST       *) override;
  TypePtr visit(StructElemDefAST    *) override;
  TypePtr visit(EnumElemAST         *) override;
  TypePtr visit(BinaryStmt          *) override;
  TypePtr visit(UnaryStmt           *) override;
  TypePtr visit(CastStmt            *) override;
  TypePtr visit(IndexAST            *) override;
  TypePtr visit(AccessAST           *) override;
  TypePtr visit(CompoundStmt        *) override;
  TypePtr visit(IfElseStmt          *) override;
  TypePtr visit(CallStmt            *) override;
  TypePtr visit(ProtoTypeAST        *) override;
  TypePtr visit(FunctionDefAST      *) override;
  TypePtr visit(FuncParamAST        *) override;
  TypePtr visit(PrimTypeAST         *) override;
  TypePtr visit(WhileStmt           *) override;
  TypePtr visit(StructTypeAST       *) override;
  TypePtr visit(EnumTypeAST         *) override;
  TypePtr visit(ConstTypeAST        *) override;
  TypePtr visit(PointerTypeAST      *) override;
  TypePtr visit(UserTypeAST         *) override;
  TypePtr visit(TranslationUnitDecl *) override;

  void Analyze() { _rootNode->SemAnalyze(this); }

};


}

#endif //LAVA_SEMA_H
