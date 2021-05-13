#include "define/ast.h"

#include <iomanip>
#include <tuple>
#include <utility>
#include <sstream>

#include "lib/guard.h"
#include "lib/strprint.h"

using namespace lava::define;
using namespace lava::mid;

// some magic
#define AST(name, ...) auto ast = DumpAST(os, #name, ##__VA_ARGS__)
#define ATOM(name, ...) DumpSimpleAST(os, #name, ##__VA_ARGS__)
#define AST_ATTR(name) std::make_tuple(#name, name##_)
#define AST_ENUM_ATTR(name, arr) \
  std::make_tuple(#name, arr[static_cast<int>(name##_)])
#define ATTR(name)                   \
  do {                               \
    auto attr = DumpAttr(os, #name); \
    name##_->Dump(os);               \
  } while (0)
#define ATTR_NULL(name)              \
  do {                               \
    auto attr = DumpAttr(os, #name); \
    DumpNullable(os, name##_);       \
  } while (0)
#define LIST_ATTR(name)                        \
  do {                                         \
    auto attr = DumpAttr(os, #name);           \
    for (const auto &i : name##_) i->Dump(os); \
  } while (0)
#define LIST_ATTR_NULL(name)                           \
  do {                                                 \
    auto attr = DumpAttr(os, #name);                   \
    for (const auto &i : name##_) DumpNullable(os, i); \
  } while (0)


namespace {

int indent_count = 0;

const auto indent = [](std::ostream &os) {
  if (indent_count) {
    os << std::setw(indent_count * 2) << std::setfill(' ') << ' ';
  }
};

std::ostream &operator<<(std::ostream &os, decltype(indent) func) {
  func(os);
  return os;
}

template <typename... Attrs>
void UnfoldAttrs(std::ostream &os, Attrs &&... attrs) {}

template <typename T, typename... Attrs>
void UnfoldAttrs(std::ostream &os, T &&attr, Attrs &&... attrs) {
  auto &&[n, v] = attr;
  os << ' ' << n << "=\"" << v << "\"";
  UnfoldAttrs(os, std::forward<Attrs>(attrs)...);
}

template <typename... Attrs>
xstl::Guard DumpAST(std::ostream &os, std::string_view name,
                    Attrs &&... attrs) {
  // dump starting tag and name
  os << indent << "<ast name=\"" << name << "\"";
  // dump inline attributes
  UnfoldAttrs(os, std::forward<Attrs>(attrs)...);
  os << '>' << std::endl;
  // increase indent num
  ++indent_count;
  return xstl::Guard([&os]() {
    // decrease indent num
    --indent_count;
    // dump closing tag
    os << indent << "</ast>" << std::endl;
  });
}

template <typename... Attrs>
void DumpSimpleAST(std::ostream &os, std::string_view name,
                   Attrs &&... attrs) {
  // dump starting tag and name
  os << indent << "<ast name=\"" << name << "\"";
  // dump inline attributes
  UnfoldAttrs(os, std::forward<Attrs>(attrs)...);
  os << " />" << std::endl;
}

xstl::Guard DumpAttr(std::ostream &os, std::string_view name) {
  // dump starting tag and name
  os << indent << "<attr name=\"" << name << "\">" << std::endl;
  // increase indent num
  ++indent_count;
  return xstl::Guard([&os]() {
    // decrease indent num
    --indent_count;
    // dump closing tag
    os << indent << "</attr>" << std::endl;
  });
}

void DumpNullable(std::ostream &os, const ASTPtr &ast) {
  if (!ast) {
    os << indent << "<null />" << std::endl;
  }
  else {
    ast->Dump(os);
  }
}

}  // namespace

/*--------------                        Dump AST                        --------------*/

void TranslationUnitDecl::Dump(std::ostream &os) const {
  AST(TranslationUnitDecl);
  LIST_ATTR(decls);
}

void VariableDecl::Dump(std::ostream &os) const {
  AST(VarDecl);
  ATTR(type);
  LIST_ATTR(defs);
}

void VariableDefAST::Dump(std::ostream &os) const {
  AST(VarDef, AST_ATTR(id));
  LIST_ATTR(arr_lens);
  ATTR_NULL(init);
}

void InitListAST::Dump(std::ostream &os) const {
  AST(InitList);
  LIST_ATTR(exprs);
}

void ProtoTypeAST::Dump(std::ostream &os) const {
  AST(FuncDecl, AST_ATTR(id));
  ATTR(type);
  LIST_ATTR(params);
}

void FunctionDefAST::Dump(std::ostream &os) const {
  AST(FuncDef);
  ATTR(header);
  ATTR(body);
}

void FuncParamAST::Dump(std::ostream &os) const {
  AST(FuncParam, AST_ATTR(id));
  ATTR(type);
  LIST_ATTR_NULL(arr_lens);
}

void StructDefAST::Dump(std::ostream &os) const {
  AST(StructDef, AST_ATTR(id));
  LIST_ATTR(elems);
}

void EnumDefAST::Dump(std::ostream &os) const {
  AST(EnumDef, AST_ATTR(id));
  LIST_ATTR(elems);
}

void TypeAliasAST::Dump(std::ostream &os) const {
  AST(TypeAlias, AST_ATTR(id));
  ATTR(type);
}

void StructElemAST::Dump(std::ostream &os) const {
  AST(StructElem);
  ATTR(type);
  LIST_ATTR(defs);
}

void StructElemDefAST::Dump(std::ostream &os) const {
  AST(StructElemDef, AST_ATTR(id));
  LIST_ATTR(arr_lens);
}

void EnumElemAST::Dump(std::ostream &os) const {
  AST(EnumElem, AST_ATTR(id));
  ATTR_NULL(expr);
}

void CompoundStmt::Dump(std::ostream &os) const {
  AST(Block);
  LIST_ATTR(stmts);
}

void IfElseStmt::Dump(std::ostream &os) const {
  AST(IfElse);
  ATTR(cond);
  ATTR(then);
  ATTR_NULL(else_then);
}

void WhileStmt::Dump(std::ostream &os) const {
  AST(While);
  ATTR(cond);
  ATTR(body);
}

void ControlAST::Dump(std::ostream &os) const {
  const char *kType[] = {"Break", "Continue", "Return"};
  AST(Control, AST_ENUM_ATTR(type, kType));
  ATTR_NULL(expr);
}

void BinaryStmt::Dump(std::ostream &os) const {
  const char *kOp[] = {
    "Add", "Sub", "Mul", "Div", "Rem",
    "And", "Or", "Xor", "Shl", "Shr",
    "LAnd", "LOr",
    "Equal", "NotEqual", "Less", "LessEq", "Great", "GreatEq",
    "Assign",
    "AssAdd", "AssSub", "AssMul", "AssDiv", "AssRem",
    "AssAnd", "AssOr", "AssXor", "AssShl", "AssShr",
  };
  AST(Binary, AST_ENUM_ATTR(op, kOp));
  ATTR(lhs);
  ATTR(rhs);
}

void CastStmt::Dump(std::ostream &os) const {
  AST(Cast);
  ATTR(type);
  ATTR(expr);
}

void UnaryStmt::Dump(std::ostream &os) const {
  const char *kOp[] = {"Pos", "Neg", "Not", "LNot", "Deref", "Addr"};
  AST(Unary, AST_ENUM_ATTR(op, kOp));
  ATTR(opr);
}

void IndexAST::Dump(std::ostream &os) const {
  AST(Index);
  ATTR(expr);
  ATTR(index);
}

void CallStmt::Dump(std::ostream &os) const {
  AST(FuncCall);
  ATTR(expr);
  LIST_ATTR(args);
}

void AccessAST::Dump(std::ostream &os) const {
  AST(Access, AST_ATTR(is_arrow), AST_ATTR(id));
  ATTR(expr);
}

void IntAST::Dump(std::ostream &os) const {
  ATOM(Int, AST_ATTR(value));
}

void CharAST::Dump(std::ostream &os) const {
  std::ostringstream oss;
  utils::DumpChar(oss, c_);
  ATOM(Char, std::make_tuple("c", oss.str()));
}

void StringAST::Dump(std::ostream &os) const {
  std::ostringstream oss;
  utils::DumpStr(oss, str_);
  ATOM(String, std::make_tuple("str", oss.str()));
}

void VariableAST::Dump(std::ostream &os) const {
  ATOM(Id, AST_ATTR(id));
}

void PrimTypeAST::Dump(std::ostream &os) const {
  const char *kType[] = {"Void", "Int8", "Int32", "UInt8", "UInt32"};
  ATOM(PrimType, AST_ENUM_ATTR(type, kType));
}

void StructTypeAST::Dump(std::ostream &os) const {
  ATOM(StructType, AST_ATTR(id));
}

void EnumTypeAST::Dump(std::ostream &os) const {
  ATOM(EnumType, AST_ATTR(id));
}

void ConstTypeAST::Dump(std::ostream &os) const {
  AST(ConstType);
  ATTR(base);
}

void PointerTypeAST::Dump(std::ostream &os) const {
  AST(PointerType, AST_ATTR(depth));
  ATTR(base);
}

void UserTypeAST::Dump(std::ostream &os) const {
  ATOM(UserType, AST_ATTR(id));
}


/*--------------                        Generate IR                        --------------*/

SSAPtr TranslationUnitDecl::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr VariableDecl::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr VariableDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr InitListAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr ProtoTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr FunctionDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr FuncParamAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr StructDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr EnumDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr TypeAliasAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr StructElemAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr StructElemDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr EnumElemAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr CompoundStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr IfElseStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr WhileStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr ControlAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr BinaryStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr CastStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr UnaryStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr IndexAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr CallStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr AccessAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr IntAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr CharAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr StringAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr VariableAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr PrimTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr StructTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr EnumTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr ConstTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr PointerTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}

SSAPtr UserTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return lava::mid::SSAPtr();
}