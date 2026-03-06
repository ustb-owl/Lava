#ifndef LAVA_COMPILER_H
#define LAVA_COMPILER_H

#include <cassert>
#include <iostream>
#include <istream>
#include <ostream>

#include "front/lexer.h"
#include "front/parser.h"
#include "mid/walker/analyzer/analyzer.h"
#include "mid/walker/analyzer/eval.h"
#include "mid/walker/irbuilder/irbuilder.h"
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
public:
  Compiler()
      : _parser(_lexer), _analysis(_eval), _irbuilder(nullptr) {
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

  // generate code
  void CodeGeneAction(bool no_ra);

  // dump asm
  void DumpASM(std::ostream &os) const { _codegen.DumpASM(os); }

  // getters
  define::ASTPtr &ast() { return _parser.ast(); }
  mid::Module &module() { return _irbuilder->module(); }
  const mid::Module &module() const { return _irbuilder->module(); }
};

}

#endif //LAVA_COMPILER_H
