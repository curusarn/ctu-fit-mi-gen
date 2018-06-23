#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>

using namespace llvm;

namespace {
    struct Hello : public FunctionPass {
        static char ID;
        Hello() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            errs() << "Hello: ";
            errs().write_escaped(F.getName()) << '\n';
            return false;
        }
    }; // end of struct Hello

    template <class T>
    struct AbstractInterpretation : public FunctionPass {
        std::map<std::string, bool> bb_data_changed_;
        std::map<Instruction*, T> inst_data_;

        explicit AbstractInterpretation(char ID) : FunctionPass(ID) {} 

        virtual T processInstruction(const Instruction * inst,
                                     const T & prev_inst_data) = 0;

        bool doInitialization(Function &) {
            bb_data_changed_.clear();
            inst_data_.clear();
            return false;
        }

        bool doFinalization(Function &) {
            bb_data_changed_.clear();
            inst_data_.clear();
            return false;
        }

        bool runOnBlockRecursively(BasicBlock &B, T & prev_inst_data) {
            errs() << "AI block: ";
            errs().write_escaped(B.getName()) << '\n';

            if (bb_data_changed_.count(B.getName())
                    and !bb_data_changed_[B.getName()]) {
                return false;
            }

            bool modified = false;  
            bool bb_data_changed = false;
            
            // loop and shit
            for (Instruction &I : B) {
                Instruction * inst = &I;
                errs().write_escaped(inst->getName()) << '\n';

                if (!inst_data_.count(inst))
                    inst_data_[inst] = T();
                    
                T inst_data = this->processInstruction(inst, prev_inst_data);
                prev_inst_data = inst_data;
              
                if (inst_data_[inst] < inst_data) {
                    inst_data_[inst] = inst_data;
                    bb_data_changed = true;
                } else
                    assert(inst_data_[inst] == inst_data
                           && "Broken lattice property:"
                           " inst_olddata > inst_newdata");
            }

            bb_data_changed_[B.getName()] = bb_data_changed;

            // continue with next bb's
            TerminatorInst * t_inst = B.getTerminator();

            for (llvm::BasicBlock * bb_next : t_inst->successors())
                modified = (runOnBlockRecursively(*bb_next, prev_inst_data)
                            || modified);

            return modified;
        }

        bool runOnFunction(Function &F) override {
            errs() << "AI function: ";
            errs().write_escaped(F.getName()) << '\n';
            bool modified=false;  

            T temp;
            return runOnBlockRecursively(F.getEntryBlock(), temp);

            for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It)
            {
                Instruction *inst = &*It;
                // if something then modify or delete instruction
            }

            return modified;
        }
    }; // end of struct AbstractInterpretation

    struct DummyAI : public AbstractInterpretation<bool> {
        static char ID;
        
        DummyAI() : AbstractInterpretation(ID) {}

        bool processInstruction(const Instruction *, const bool &) override {
            return true; 
        }

        bool doInitialization(Function &F) {
            bool ret = AbstractInterpretation::doInitialization(F);
            // do stuff
            return ret;
        }

        bool doFinalization(Function &F) {
            // do stuff
            return AbstractInterpretation::doFinalization(F);
        }
    }; // end of DummyAI


    struct CheckNames : public FunctionPass {
        static char ID;
        CheckNames() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            errs() << "CheckNames: ";
            errs().write_escaped(F.getName()) << '\n';
            Function::BasicBlockListType & bb_list = F.getBasicBlockList();

            for (llvm::BasicBlock & bb : bb_list) {
                errs().write_escaped(bb.getName()) << '\n';

                for (llvm::Instruction & inst : bb) {
                    errs() << "    ";
                    errs().write_escaped(inst.getName()) << '\n';
                }
            }
            return false;
        }
    }; // end of struct CheckNames
}  // end of anonymous namespace


char Hello::ID = 0;
static RegisterPass<Hello> X1("hello", "Hello World Pass",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);

char CheckNames::ID = 1;
static RegisterPass<CheckNames> X2("checknames", "CheckNames Pass",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);

char DummyAI::ID = 2;
static RegisterPass<DummyAI> X3("ai_dummy", "Dummy AbstractInterpretation Pass",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);


