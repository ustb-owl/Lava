#ifndef LAVA_IRBUILDER_H
#define LAVA_IRBUILDER_H

#include "mid/ir/ssa.h"
#include "mid/ir/module.h"
#include "mid/ir/usedef/value.h"
#include "mid/visitor/visitor.h"

namespace lava::mid {

class IRBuilder : Visitor<SSAPtr> {
private:
  bool       _in_func;
  Module     _module;
  ASTPtr    &_translation_decl_unit;

public:
  explicit IRBuilder(ASTPtr &ast)
    : _translation_decl_unit(ast) { _in_func = false; }

  virtual ~IRBuilder() = default;

  // get module
  Module &module() { return _module; }

  // new value environment
  xstl::Guard NewEnv() { return _module.NewEnv(); }

  SSAPtr visit(IntAST              *) override;
  SSAPtr visit(CharAST             *) override;
  SSAPtr visit(StringAST           *) override;
  SSAPtr visit(VariableAST         *) override;
  SSAPtr visit(VariableDecl        *) override;
  SSAPtr visit(VariableDefAST      *) override;
  SSAPtr visit(InitListAST         *) override; // new
  SSAPtr visit(StructDefAST        *) override; // new
  SSAPtr visit(EnumDefAST          *) override;
  SSAPtr visit(TypeAliasAST        *) override; // new
  SSAPtr visit(StructElemAST       *) override;
  SSAPtr visit(StructElemDefAST    *) override;
  SSAPtr visit(EnumElemAST         *) override;
  SSAPtr visit(BinaryStmt          *) override;
  SSAPtr visit(UnaryStmt           *) override;
  SSAPtr visit(CastStmt            *) override;
  SSAPtr visit(IndexAST            *) override;
  SSAPtr visit(AccessAST           *) override;
  SSAPtr visit(CompoundStmt        *) override;
  SSAPtr visit(IfElseStmt          *) override;
  SSAPtr visit(CallStmt            *) override;
  SSAPtr visit(ControlAST          *) override;
  SSAPtr visit(ProtoTypeAST        *) override;
  SSAPtr visit(FunctionDefAST      *) override;
  SSAPtr visit(FuncParamAST        *) override;
  SSAPtr visit(PrimTypeAST         *) override;
  SSAPtr visit(WhileStmt           *) override;
  SSAPtr visit(StructTypeAST       *) override;
  SSAPtr visit(EnumTypeAST         *) override;
  SSAPtr visit(ConstTypeAST        *) override;
  SSAPtr visit(PointerTypeAST      *) override;
  SSAPtr visit(UserTypeAST         *) override;
  SSAPtr visit(TranslationUnitDecl *) override;

  void EmitIR() { _translation_decl_unit->CodeGeneAction(this); }

  // helper
  void SetInsertPointAtEntry();
  void SetInsertPoint(const BlockPtr &BB);

    // print error message
  SSAPtr LogError(const front::LoggerPtr &log, std::string &message);
  SSAPtr LogError(const front::LoggerPtr &log, std::string &message, const std::string &id);
};

}

#endif //LAVA_IRBUILDER_H
