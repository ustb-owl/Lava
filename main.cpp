#include <fstream>
#include <sstream>
#include "driver/compiler.h"

using namespace std;
using namespace lava::driver;

int main(int argc, char *argv[]) {
  std::string file = argv[1];

  Compiler comp;

  ifstream ifs(file);
  comp.Open(&ifs);
  comp.Parse();

//  auto &rootNode = comp.ast();
//  rootNode->Dump(std::cout);

  comp.EmitIR();
  comp.RunPasses();
  comp.DumpIR(std::cout);

  comp.CodeGeneAction();
  comp.DumpASM(std::cout);
  return 0;
}