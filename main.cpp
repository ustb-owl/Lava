#include <fstream>
#include <sstream>
#include "version.h"
#include "driver/compiler.h"
#include "lib/argsparser.h"

using namespace std;
using namespace lava::driver;
using namespace argparse;

enum class OPTION {
  NONE,
  DUMP_AST,
  DUMP_IR,
  DUMP_ASM,
};

int main(int argc, const char *argv[]) {
  OPTION option = OPTION::NONE;
  std::ostream *os = &cout;

  // parser command line arguments
  ArgumentParser parser("lacc", "lacc parser");

  parser.add_argument()
      .names({"-o", "--output"})
      .description("set output file name")
      .required(false);

  parser.add_argument()
      .names({"-S", "--dump-asm"})
      .description("generate asm file")
      .required(false);

  parser.add_argument()
      .names({"-I", "--dump-ir"})
      .description("generate ir file")
      .required(false);

  parser.add_argument()
      .names({"-T", "--dump-ast"})
      .description("generate ast file")
      .required(false);

  parser.add_argument("filename", "input filename")
      .position(ArgumentParser::Argument::Position::LAST).required(true);
  parser.enable_help();

  auto err = parser.parse(argc, argv);
  if (err) {
    parser.print_help();
    return -1;
  }

  if (parser.exists("help")) {
    parser.print_help();
    return 0;
  }

  if (parser.exists("S")) {
    option = OPTION::DUMP_ASM;
  }

  if (parser.exists("I")) {
    option = OPTION::DUMP_IR;
  }

  if (parser.exists("T")) {
    option = OPTION::DUMP_AST;
  }

  std::string output_file;
  if (parser.exists("o")) {
    std::ofstream *o_file;
    output_file = parser.get<std::string>("o");
    o_file = new ofstream(output_file, std::fstream::out | std::fstream::trunc);
    if (!o_file->is_open()) ERROR("open output file ");
    os = o_file;
  }

  auto file = parser.get<std::string>("filename");

//#if 0

  Compiler comp;

  ifstream ifs(file);
  if (!ifs.is_open()) ERROR("open file %s failed", file.c_str());

  comp.Open(&ifs);

  // parse
  comp.Parse();

  // generate IR and perform optimization
  comp.EmitIR();
  comp.RunPasses();

  // code generation
  comp.CodeGeneAction();

  switch (option) {
    case OPTION::NONE:
      break;
    case OPTION::DUMP_AST: {
      auto &rootNode = comp.ast();
      rootNode->Dump(*os);
      break;
    }
    case OPTION::DUMP_IR: {
      comp.DumpIR(*os);
      break;
    }
    case OPTION::DUMP_ASM: {
      comp.DumpASM(*os);
      break;
    }
  }

  return 0;
//#endif
}