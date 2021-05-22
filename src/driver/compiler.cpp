#include "compiler.h"
#include "define/builtin.h"
#include <sstream>

using namespace lava::opt;

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
  // pre-build builtin functions
  auto pre_ast = PreBuild();
  BaseAST *raw = pre_ast.get();
  auto *trans = static_cast<TranslationUnitDecl *>(raw);

  // merge two ast node
  _parser.Parse();
  auto &rootNode = _parser.ast();
  rootNode->Merge(trans->decls());

  _parser.SemaAnalysis(_analysis);
  if (_irbuilder == nullptr) {
    _irbuilder = new mid::IRBuilder(_parser.ast());
  }
}

void Compiler::RunPasses() {
  PassManager::Initialize();
  PassManager::SetModule(_irbuilder->module());
  PassManager::RunPasses();
}

ASTPtr Compiler::PreBuild() {
  std::istringstream iss;
  iss.str(
      "void memset(int *dst, int value, int size);\n"
      "int getint();\n"
      "int getch();\n"
      "int getarray(int a[]);\n"
      "void putint(int a);\n"
      "void putch(int a);\n"
      "void putarray(int n, int a[]);\n"
      "void starttime();\n"
      "void stoptime();\n"
      "void memcpy(int *a, int *b, int size);\n"
  );
  front::Lexer  tmp_lex(&iss);
  front::Parser tmp_paser(tmp_lex);

  tmp_paser.Parse();
  return std::move(tmp_paser.ast());
}

}