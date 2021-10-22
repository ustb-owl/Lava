#ifndef LAVA_MID_VISITOR_H
#define LAVA_MID_VISITOR_H

#include "define/ast.h"


namespace lava::mid {
using namespace lava::define;

template<typename RETURN_TYPE>
class Visitor {
public:
  Visitor() = default;
  ~Visitor() = default;

  virtual RETURN_TYPE visit(IntAST              *) = 0;
  virtual RETURN_TYPE visit(CharAST             *) = 0;
  virtual RETURN_TYPE visit(StringAST           *) = 0;
  virtual RETURN_TYPE visit(VariableAST         *) = 0;
  virtual RETURN_TYPE visit(VariableDecl        *) = 0;
  virtual RETURN_TYPE visit(VariableDefAST      *) = 0;
  virtual RETURN_TYPE visit(InitListAST         *) = 0;
  virtual RETURN_TYPE visit(StructDefAST        *) = 0;
  virtual RETURN_TYPE visit(EnumDefAST          *) = 0;
  virtual RETURN_TYPE visit(TypeAliasAST        *) = 0;
  virtual RETURN_TYPE visit(StructElemAST       *) = 0;
  virtual RETURN_TYPE visit(StructElemDefAST    *) = 0;
  virtual RETURN_TYPE visit(EnumElemAST         *) = 0;
  virtual RETURN_TYPE visit(BinaryStmt          *) = 0;
  virtual RETURN_TYPE visit(UnaryStmt           *) = 0;
  virtual RETURN_TYPE visit(CastStmt            *) = 0;
  virtual RETURN_TYPE visit(IndexAST            *) = 0;
  virtual RETURN_TYPE visit(AccessAST           *) = 0;
  virtual RETURN_TYPE visit(CompoundStmt        *) = 0;
  virtual RETURN_TYPE visit(IfElseStmt          *) = 0;
  virtual RETURN_TYPE visit(CallStmt            *) = 0;
  virtual RETURN_TYPE visit(ControlAST          *) = 0;
  virtual RETURN_TYPE visit(ProtoTypeAST        *) = 0;
  virtual RETURN_TYPE visit(FunctionDefAST      *) = 0;
  virtual RETURN_TYPE visit(FuncParamAST        *) = 0;
  virtual RETURN_TYPE visit(PrimTypeAST         *) = 0;
  virtual RETURN_TYPE visit(WhileStmt           *) = 0;
  virtual RETURN_TYPE visit(StructTypeAST       *) = 0;
  virtual RETURN_TYPE visit(EnumTypeAST         *) = 0;
  virtual RETURN_TYPE visit(ConstTypeAST        *) = 0;
  virtual RETURN_TYPE visit(PointerTypeAST      *) = 0;
  virtual RETURN_TYPE visit(UserTypeAST         *) = 0;
  virtual RETURN_TYPE visit(TranslationUnitDecl *) = 0;
};
}

#endif //LAVA_VISITOR_H
