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
#include "opt/pass_manager.h"
#include "back/codegen.h"

namespace lava::driver {

class Compiler {
private:
  front::Lexer         _lexer;
  front::Parser        _parser;
  mid::Analyzer        _analysis;
  mid::Evaluator       _eval;
  mid::IRBuilder      *_irbuilder;
  back::CodeGenerator  _codegen;
  std::string          _file;

  bool            _dump_ast, _dump_ir;
  bool            _dump_pass_info, _dump_code;

  bool            _opt_flag;
  bool            _no_ra;
  std::ostream   *_os;
public:
  Compiler()
      : _parser(_lexer), _analysis(_eval), _irbuilder(nullptr),
        _dump_ast(false), _dump_ir(false),
        _dump_pass_info(false), _dump_code(false),
        _opt_flag(false), _no_ra(false),
        _os(&std::cout) {
    Reset();
  }

  ~Compiler() {
    delete _irbuilder;
  }

  // reset compiler
  void Reset();

  // pre-build
  ASTPtr PreBuild();

  // open stream
  void Open(std::istream *in);

  // set file
  void SetFile(const std::string &file) { _file = file; }

  // generate ast
  void Parse();

  // Emit IR
  void EmitIR() { _irbuilder->EmitIR(); }

  // dump IR
  void DumpIR(std::ostream &os) const { _irbuilder->module().Dump(os); }

  // dump CFG
  void DumpCFG(const std::string &output_name) const;

  // run passes on IR
  void RunPasses();

  // generate code
  void CodeGeneAction();

  // dump asm
  void DumpASM(std::ostream &os) const { _codegen.DumpASM(os); }

  // setters
  void set_dump_ast(bool dump_ast)   { _dump_ast  = dump_ast;  }
  void set_dump_ir(bool dump_ir)     { _dump_ir   = dump_ir;   }
  void set_dump_code(bool dump_code) { _dump_code = dump_code; }
  void set_opt_flat(bool flag)       { _opt_flag  = flag;      }

  void set_dump_pass_info(bool dump_pass_info) {
    _dump_pass_info = dump_pass_info;
  }

  void set_ostream(std::ostream *os) {
    DBG_ASSERT(os != nullptr, "os pointer is nullptr");
    _os = os;
  }

  // getters
  define::ASTPtr &ast() { return _parser.ast(); }

  void no_ra(bool no_ra) { _no_ra = no_ra; }
};

}

#endif //LAVA_COMPILER_H
