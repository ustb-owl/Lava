#ifndef LAVA_FRONT_TOKEN_H_
#define LAVA_FRONT_TOKEN_H_

#include <cassert>

// all supported keywords
#define LAVA_KEYWORDS(e) \
  e(Struct, "struct") e(Enum, "enum") e(TypeDef, "typedef") \
  e(If, "if") e(Else, "else") e(While, "while") e(SizeOf, "sizeof") \
  e(Break, "break") e(Continue, "continue") e(Return, "return") \
  e(Void, "void") e(Unsigned, "unsigned") \
  e(Int, "int") e(Bool, "bool") e(Char, "char") e(Const, "const")

// all supported operators
#define LAVA_OPERATORS(e) \
  e(Add, "+", 90) e(Sub, "-", 90) e(Mul, "*", 100) e(SDiv, "/", 100) \
  e(SRem, "%", 100) e(Equal, "==", 60) e(NotEqual, "!=", 60) \
  e(SLess, "<", 70) e(SLessEq, "<=", 70) e(SGreat, ">", 70) \
  e(SGreatEq, ">=", 70) e(LogicAnd, "&&", 20) e(LogicOr, "||", 10) \
  e(LogicNot, "!", -1) e(And, "&", 50) e(Or, "|", 30) e(Not, "~", -1) \
  e(Xor, "^", 40) e(Shl, "<<", 80) e(LShr, ">>", 80) \
  e(Access, ".", -1) e(Arrow, "->", -1) \
  e(Assign, "=", 0) e(AssAdd, "+=", 0) e(AssSub, "-=", 0) \
  e(AssMul, "*=", 0) e(AssSDiv, "/=", 0) e(AssSRem, "%=", 0) \
  e(AssAnd, "&=", 0) e(AssOr, "|=", 0) e(AssXor, "^=", 0) \
  e(AssShl, "<<=", 0) e(AssLShr, ">>=", 0)

// expand first element to comma-separated list
#define LAVA_EXPAND_FIRST(i, ...)       i,
// expand second element to comma-separated list
#define LAVA_EXPAND_SECOND(i, j, ...)   j,
// expand third element to comma-separated list
#define LAVA_EXPAND_THIRD(i, j, k, ...) k,

namespace lava::front {

enum class Token {
  Error, End,
  Id, Int, String, Char,
  Keyword, Operator,
  Other,
};

enum class Keyword { LAVA_KEYWORDS(LAVA_EXPAND_FIRST) };
enum class Operator {
  LAVA_OPERATORS(LAVA_EXPAND_FIRST)

  // add unsigned operator
  UDiv, URem, ULess, UGreat, ULessEq, UGreatEq, AShr,
  AssAShr, AssUDiv, AssURem,

  // add unary operator
  Pos, Neg, Deref, Addr, SizeOf
};

}  // namespace lava::front

#endif  // LAVA_FRONT_TOKEN_H_
