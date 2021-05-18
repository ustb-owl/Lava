#ifndef LAVA_COMPILER_H
#define LAVA_COMPILER_H

#include <istream>
#include <ostream>
#include <iostream>
#include <cassert>

#include "front/lexer.h"
#include "front/parser.h"
#include "mid/walker/analyzer/analyzer.h"
#include "mid/walker/analyzer/eval.h"
#include "mid/walker/irbuilder/irbuilder.h"

namespace lava::driver {

class Compiler {
private:
  front::Lexer    _lexer;
  front::Parser   _parser;
  mid::Analyzer   _analysis;
  mid::Evaluator  _eval;
  mid::IRBuilder *_irbuilder;

  bool            _dump_ast, _dump_ir;
  bool            _dump_pass_info, _dump_code;
  std::ostream   *_os;
public:
  Compiler()
      : _parser(_lexer), _analysis(_eval), _irbuilder(nullptr),
        _dump_ast(false), _dump_ir(false),
        _dump_pass_info(false), _dump_code(false),
        _os(&std::cout) {
    Reset();
  }

  // reset compiler
  void Reset();

  // open stream
  void Open(std::istream *in);

  // generate ast
  void Parse();

  // Emit IR
  void EmitIR() { _irbuilder->EmitIR(); }

  // setters
  void set_dump_ast(bool dump_ast)   { _dump_ast  = dump_ast;  }
  void set_dump_yuir(bool dump_yuir) { _dump_ir   = dump_yuir; }
  void set_dump_code(bool dump_code) { _dump_code = dump_code; }

  void set_dump_pass_info(bool dump_pass_info) {
    _dump_pass_info = dump_pass_info;
  }

  void set_ostream(std::ostream *os) {
    DBG_ASSERT(os != nullptr, "os pointer is nullptr");
    _os = os;
  }

  // getters
  define::ASTPtr &ast() { return _parser.ast(); }
};

}

#endif //LAVA_COMPILER_H