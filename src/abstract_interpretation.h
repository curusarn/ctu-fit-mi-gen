#ifndef ABSTRACT_INTERPRETATION
#define ABSTRACT_INTERPRETATION

#include "llvm.h"

class AbstractInterpretation {
public:
    static void dummy(llvm::Function * mainFunc, bool verbose);
};
    
struct Hello : public llvm::FunctionPass {
    static char ID;
    Hello() : llvm::FunctionPass(ID) {}

    bool runOnFunction(llvm::Function &F) override {
        llvm::errs() << "Hello: ";
        llvm::errs().write_escaped(F.getName()) << '\n';
        return false;
    }
}; // end of struct Hello


// templated <abs_domain, abs_semantics>

// abs_semantics (llvm::instruction)
//
// switch 
//      case instruction1:
//          handle inst1
//          break;
//
//      case inst2:
//          handle inst2
//          break;
// 

#endif // ABSTRACT_INTERPRETATION
