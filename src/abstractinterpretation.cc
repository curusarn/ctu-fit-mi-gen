
#include "abstractinterpretation.h"
#include <iostream>

void AbstractInterpretation::dummy(llvm::Function * mainFunc,
                                   bool verbose) {
    if (verbose)
        std::cout << "AbstractInterpretation" << std::endl;

    llvm::Function::BasicBlockListType & bb_list = 
                                                mainFunc->getBasicBlockList();


    std::cout << "[AI] print basic blocks (and name the unnamed)" << std::endl;
    std::string bb_name_prefix = "BB_";
    std::string inst_name_prefix = "INST_";
    int b = 0, i = 10;
    for (llvm::BasicBlock & bb : bb_list) {
        std::string bb_name = bb.getName();
        if (bb_name == std::string("")) {
            bb_name = bb_name_prefix + std::to_string(b);
            b++;
            bb.setName(bb_name);
        }
        std::cout << bb_name << std::endl;
        //eraseFromParent()
        //phis ()

        for (llvm::Instruction & inst : bb) {
            std::string inst_name = inst.getName();
            if (inst_name == std::string("")) {
                inst_name = inst_name_prefix + std::to_string(i);
                i++;
                inst.setName(inst_name);
            }
            std::cout << "    " << inst_name << std::endl;
            //eraseFromParent()
        }
    }
    
    std::cout << "[AI] walk trough basic blocks" << std::endl;
    
    auto ai = AbstractInterpretation();
    ai.visitBasicBlock(mainFunc->getEntryBlock());

}

void AbstractInterpretation::visitBasicBlock(const llvm::BasicBlock & bb) {
    std::string bb_name = bb.getName();
    std::cout << bb_name << std::endl;
    if (visited_.count(bb_name))
        return;

    std::cout << "    -> true" << std::endl;
    visited_[bb_name] = true;

    const llvm::TerminatorInst * t_inst = bb.getTerminator();

    for (const llvm::BasicBlock * bb_next : t_inst->successors()) 
        visitBasicBlock(*bb_next);
}
