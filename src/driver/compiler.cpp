#include "compiler.h"


namespace lava::driver {

void Compiler::Reset() {
  // reset lexer & parser
  _parser.Reset();
  // reset the rest part
  _analysis.Reset();
  _eval.Reset();
}

void Compiler::Open(std::istream *in) {
  // reset lexer & parser only
  _lexer.Reset(in);
  _parser.Reset();
}

void Compiler::Parse() {
  _parser.Parse();
}

}