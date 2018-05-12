#ifndef ABSTRACTINTERPRETATION_H
#define ABSTRACTINTERPRETATION_H

#include "llvm.h"

class AbstractInterpretation {
    std::map<std::string, bool> visited_;
public:
    static void dummy(llvm::Function * mainFunc, bool verbose);
    void visitBasicBlock(const llvm::BasicBlock & bb);
};
    
//struct Hello : public llvm::FunctionPass {
//    static char ID;
//    Hello() : llvm::FunctionPass(ID) {}
//
//    bool runOnFunction(llvm::Function &F) override {
//        llvm::errs() << "Hello: ";
//        llvm::errs().write_escaped(F.getName()) << '\n';
//        return false;
//    }
//}; // end of struct Hello


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

#endif // ABSTRACTINTERPRETATION_H
