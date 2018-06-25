#include "llvm/IR/CFG.h"
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

    template <class T> struct AbstractInterpretation : public FunctionPass {
        using state_type = std::map<std::string, T>;
        std::map<std::string, std::map<std::string, T>> bb_state_;

        explicit AbstractInterpretation(char ID) : FunctionPass(ID) {} 

        virtual state_type flowState(const Instruction * inst,
                const state_type & cur_state) = 0;

        bool doInitialization(Function &) {
            bb_state_.clear();
            return false;
        }

        bool doFinalization(Function &) {
            bb_state_.clear();
            return false;
        }

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

        bool statesEqual(const state_type & A, const state_type & B) {
            return A == B;
        }

        state_type mergePredecessorStates(const BasicBlock & bb) {
            state_type state;
            for (const BasicBlock * pred : predecessors(&bb)) {
                state = mergeStates(state, bb_state_[pred->getName()]);
            }
            return state;
        }

        bool runOnFunction(Function &F) override {
            errs() << "AI function: ";
            errs().write_escaped(F.getName()) << '\n';

            bool modified=false;  
            // Set initial states
            Function::BasicBlockListType & bb_list = F.getBasicBlockList();
            for (BasicBlock & bb : bb_list) {
                bb_state_[bb.getName()] = state_type();
                //set bb.inState = bottom for every variable
                //set bb.outState = bottom for every variable
            }
            BasicBlock & ebb = F.getEntryBlock();
            //set ebb.inState = (user-defined initial state)

            // Iterate until fixed point TODO
            bool changed = true;
            while (changed) {
                changed = false;
                // Process all basic blocks, looking for changes
                for (BasicBlock & bb : bb_list) {
                    errs() << "AI block: ";
                    errs().write_escaped(bb.getName()) << '\n';
                    // Set up in state
                    state_type bb_instate = (&bb != &ebb) ? mergePredecessorStates(bb) : state_type();
                    state_type cur_state = bb_instate;

                    // Interpret block
                    for (Instruction &I : bb) {
                        Instruction * inst = &I;

                        cur_state = this->flowState(inst, cur_state);
                    }
                    // Set out state and check for changes
                    if (!statesEqual(cur_state, bb_state_.at(bb.getName())))
                        changed = true;
                    bb_state_[bb.getName()] = cur_state;
                }
            }

            return modified;

            //bool modified=false;  

            for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It)
            {
                Instruction *inst = &*It;
                // if something then modify or delete instruction
            }

            return modified;
        }


    }; // end of struct AbstractInterpretation

    struct DummyLattice {
        int value;
        static DummyLattice getTop() { return DummyLattice(1); }
        static DummyLattice getBottom() { return DummyLattice(0); }
        static DummyLattice getLeastUpperBound(const DummyLattice & A,
                const DummyLattice & B) {
            int v = std::max(A.value, B.value);
            return (v == A.value) ? DummyLattice(A) : DummyLattice(B);
        }
        DummyLattice() : DummyLattice(getBottom()) {}
        DummyLattice(const DummyLattice & dl) = default;
        DummyLattice & operator= (const DummyLattice & dl) 
        { value = dl.value; return *this; }
        explicit DummyLattice(int x) : value(x) {};
        bool operator== (const DummyLattice & B) const { return value == B.value; };
    };

    struct DummyAI : public AbstractInterpretation<DummyLattice> {
        static char ID;

        DummyAI() : AbstractInterpretation(ID) {}

        AbstractInterpretation::state_type flowState(const Instruction *,
                const AbstractInterpretation::state_type &) override {
            auto state = AbstractInterpretation::state_type();
            state["dummy"] = DummyLattice(1);
            return state;
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


