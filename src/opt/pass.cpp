#include "pass.h"

#include <ostream>
namespace lava::opt {

// print - Print out the internal state of the pass.  This is called by Analyze
// to print out the contents of an analysis.  Otherwise it is not necessary to
// implement this method.
void Pass::print(std::ostream &OS, const Module *) const {
  OS << "Pass::print not implemented for pass: '" << name() << "'!\n";
}


}