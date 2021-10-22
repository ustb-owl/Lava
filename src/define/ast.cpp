#include "define/ast.h"

#include <iomanip>
#include <tuple>
#include <utility>
#include <sstream>

#include "lib/guard.h"
#include "lib/strprint.h"
#include "mid/walker/analyzer/eval.h"
#include "mid/walker/analyzer/analyzer.h"
#include "mid/walker/irbuilder/irbuilder.h"

using namespace lava::mid;
using namespace lava::define;

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
namespace lava::define {

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
      "And", "Or", "Xor", "Shl", "LShr",
      "LAnd", "LOr",
      "Equal", "NotEqual", "Less", "LessEq", "Great", "GreatEq",
      "Assign",
      "AssAdd", "AssSub", "AssMul", "AssDiv", "AssRem",
      "AssAnd", "AssOr", "AssXor", "AssShl", "AssAShr",
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
  return irbuilder->visit(this);
}

SSAPtr VariableDecl::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr VariableDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr InitListAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr ProtoTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr FunctionDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr FuncParamAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr StructDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr EnumDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr TypeAliasAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr StructElemAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr StructElemDefAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr EnumElemAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr CompoundStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr IfElseStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr WhileStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr ControlAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr BinaryStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr CastStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr UnaryStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr IndexAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr CallStmt::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr AccessAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr IntAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr CharAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr StringAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr VariableAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr PrimTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr StructTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr EnumTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr ConstTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr PointerTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

SSAPtr UserTypeAST::CodeGeneAction(IRBuilder *irbuilder) {
  return irbuilder->visit(this);
}

TypePtr TranslationUnitDecl::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr VariableDecl::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr VariableDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr InitListAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr ProtoTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr FunctionDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr FuncParamAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr StructDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr EnumDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr TypeAliasAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr StructElemAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr StructElemDefAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr EnumElemAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr CompoundStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr IfElseStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr WhileStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr ControlAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr BinaryStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr CastStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr UnaryStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr IndexAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr CallStmt::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr AccessAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr IntAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr CharAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr StringAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr VariableAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr PrimTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr StructTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr EnumTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr ConstTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr PointerTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}

TypePtr UserTypeAST::SemaAnalyze(Analyzer &ana) {
  return ana.visit(this);
}


std::optional<std::uint32_t> TranslationUnitDecl::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> VariableDecl::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> VariableDefAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> InitListAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> ProtoTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> FunctionDefAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> FuncParamAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> StructDefAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> EnumDefAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> TypeAliasAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> StructElemAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> StructElemDefAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> EnumElemAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> CompoundStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> IfElseStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> WhileStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> ControlAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> BinaryStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> CastStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> UnaryStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> IndexAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> CallStmt::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> AccessAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> IntAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> CharAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> StringAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> VariableAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> PrimTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> StructTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> EnumTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> ConstTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> PointerTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}

std::optional<std::uint32_t> UserTypeAST::Eval(Evaluator &eval) {
  return eval.visit(this);
}


}