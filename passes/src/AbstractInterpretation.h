#ifndef MYPASSES_ABSTRACTINTERPRETATION_H
#define MYPASSES_ABSTRACTINTERPRETATION_H

#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace my_passes {

    template <class T>
    struct AbstractInterpretation : public FunctionPass {
        using state_type = std::map<std::string, T>;
        std::map<std::string, std::map<std::string, T>> bb_state_;

        explicit AbstractInterpretation(char ID) : FunctionPass(ID) {} 

        virtual state_type flowState(const Instruction * inst,
                                     state_type cur_state) = 0;

        virtual state_type getEntryBlockState(const BasicBlock & bb) {
            errs() << "WARN: using dummy getEntryBlockState\n";
            return state_type();
        };

        virtual bool postprocess(Function &F) {
            errs() << "WARN: using dummy postprocess\n";
            return false;
        };

        state_type mergeStates(const state_type & A, const state_type & B) {
            state_type state;
            for (const auto & it : A)
                if (!B.count(it.first))
                    state[it.first] = it.second; 
                else
                    state[it.first] =
                        T::getLeastUpperBound(it.second, B.at(it.first));

            for (const auto & it : B)
                if (!A.count(it.first))
                    state[it.first] = it.second; 

            return state; 
        }

        state_type mergePredecessorStates(const BasicBlock & bb) {
            state_type state;
            for (const BasicBlock * pred : predecessors(&bb)) {
                state = mergeStates(state, bb_state_[pred->getName()]);
            }
            return state;
        }

        bool runOnFunction(Function &F) override {
            bb_state_.clear();
            errs() << ">>> AI FUNC: ";
            errs().write_escaped(F.getName()) << '\n';

            bool modified=false;  
            // Set initial states
            Function::BasicBlockListType & bb_list = F.getBasicBlockList();
            for (BasicBlock & bb : bb_list) {
                bb_state_[bb.getName()] = state_type();
            }
            BasicBlock & ebb = F.getEntryBlock();
            bb_state_[ebb.getName()] = this->getEntryBlockState(ebb);

            // Iterate until fixed point
            bool changed = true;
            while (changed) {
                changed = false;
                // Process all basic blocks, looking for changes
                for (BasicBlock & bb : bb_list) {
                    errs() << ">> AI BLOCK: ";
                    errs().write_escaped(bb.getName()) << '\n';
                    // Set up in state
                    state_type cur_state = 
                                      (&bb != &ebb) ? mergePredecessorStates(bb)
                                                    : state_type();
                    // Interpret block
                    for (Instruction &I : bb) {
                        Instruction * inst = &I;

                        cur_state = this->flowState(inst, cur_state);
                    }
                    // Set out state and check for changes
                    if (cur_state != bb_state_.at(bb.getName()))
                        changed = true;
                    bb_state_[bb.getName()] = cur_state;
                }
            }
            this->postprocess(F);
            bb_state_.clear();
            return modified;
        }
    }; // end of struct AbstractInterpretation
} // end of namespace my_passes

#endif // MYPASSES_ABSTRACTINTERPRETATION_H
