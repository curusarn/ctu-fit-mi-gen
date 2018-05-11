
#include <iostream>
#include "abstract_interpretation.h"

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
        if (bb_name.compare("") == 0) {
            bb_name = bb_name_prefix + std::to_string(b);
            b++;
            bb.setName(bb_name);
        }
        std::cout << bb_name << std::endl;
        //eraseFromParent()
        //phis ()

        for (llvm::Instruction & inst : bb) {
            std::string inst_name = inst.getName();
            if (inst_name.compare("") == 0) {
                inst_name = inst_name_prefix + std::to_string(i);
                i++;
                inst.setName(inst_name);
            }
            std::cout << "    " << inst_name << std::endl;
            //eraseFromParent()
        }
    }
    
    std::cout << "[AI] walk trough basic blocks" << std::endl;
    std::map<std::string, bool> visited;
    
    //llvm::BasicBlock bb = LLVMGetEntryBasicBlock(mainFunc);
    //llvm::BasicBlock & bb

}
