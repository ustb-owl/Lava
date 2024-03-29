#include <fstream>
#include <sstream>
#include <utility>
#include "version.h"
#include "driver/compiler.h"

#include "lib/fire.hpp"

using namespace std;
using namespace lava::driver;
//using namespace argparse;
using namespace fire;

enum class OPTION {
  NONE,
  DUMP_AST,
  DUMP_IR,
  DUMP_ASM,
  DUMP_CFG,
};

int Main(bool AST = fire::arg({"-T", "--dump-ast"}),
         bool IR = fire::arg({"-I", "--dump-ir"}),
         bool ASM = fire::arg({"-S", "--dump-asm"}),
         bool CFG = fire::arg({"-C", "--dump-cfg"}),
         const fire::optional<std::string>& output = fire::arg({"-o", "--output", "set output file name"}),
         std::vector<std::string> filename = fire::arg(fire::variadic()),
         bool Opt = fire::arg({"-O", "--opt", "optimization level is {num}"}),
         bool NO_RA = fire::arg({"--no-ra", "disable register allocation"}),
         bool level = fire::arg{{"-2", "opt level"}}
         ) {

  OPTION option = OPTION::NONE;
  std::ostream *os = &cout;

  Compiler comp;
  std::string file, output_file;

  if (AST) {
    option = OPTION::DUMP_AST;
  } else if (IR) {
    option = OPTION::DUMP_IR;
  } else if (ASM) {
    option = OPTION::DUMP_ASM;
  } else if (CFG) {
    option = OPTION::DUMP_CFG;
  } else {
    ERROR("should not reach here");
  }

  if (level) comp.set_opt_flat(true);

  if (output.has_value()) {
    std::ofstream *o_file;
    output_file = output.value();
    o_file = new ofstream(output_file, std::fstream::out | std::fstream::trunc);
    if (!o_file->is_open()) ERROR("open output file ");
    os = o_file;
  }

  file = std::move(filename[0]);
  comp.SetFile(file);

  if (Opt) {
    comp.set_opt_flat(true);
  }

  // if run register allocation
  printf("nora: %d", NO_RA);
  if (NO_RA) {
    comp.no_ra(true);
  }

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
    case OPTION::DUMP_CFG: {
      comp.DumpCFG(output_file.empty() ? "ir" : output_file);
      break;
    }
  }

  return 0;
}

FIRE(Main, "Lava compiler")