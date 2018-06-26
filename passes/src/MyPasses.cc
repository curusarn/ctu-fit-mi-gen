#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
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

        static void dumpState(const state_type & state) {
            for (auto& x : state)
                std::cout << x.first << ": "
                << static_cast<int>(x.second.type) << " ("
                << x.second.value << ")\n";
        }

        virtual state_type flowState(const Instruction * inst,
                                     state_type cur_state) = 0;

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
            errs() << ">>> AI FUNC: ";
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
                    errs() << ">> AI BLOCK: ";
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
                        AbstractInterpretation::state_type state) override {
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

    struct BasicLattice {
        enum class Type {
            Top = 40,
            Any = 30,
            Positive = 20, Zero = 21, Negative = 22,
            SingleValue = 10,
            Bottom = 0
        };
        int getRank() const { return static_cast<int>(type) / 10; }

        int64_t value;
        Type type;

        BasicLattice() : BasicLattice(getBottom()) {}
        BasicLattice(const BasicLattice & x) = default;
        explicit BasicLattice(Type t, int v = 0)
            : value(v), type(t) {};

        static BasicLattice getTop()
        { return BasicLattice(Type::Top); }
        static BasicLattice getBottom()
        { return BasicLattice(Type::Bottom); }

        bool operator== (const BasicLattice & B) const {
            if (type == B.type)
                if (Type::SingleValue == type)
                    return value == B.value;
                return true;

            return false;
        };

        BasicLattice getParent() const {
            switch (type) {
                case Type::Bottom:
                    assert(false && "Bottom of the lattice technically has"
                                    " a parent but why would you do this.");

                case Type::SingleValue:
                    if (value > 0)
                        return BasicLattice(Type::Positive);
                    if (value < 0)
                        return BasicLattice(Type::Negative);
                    return BasicLattice(Type::Zero);

                case Type::Positive:
                case Type::Negative:
                case Type::Zero:
                    return BasicLattice(Type::Any);

                case Type::Any:
                    return BasicLattice(Type::Top);

                case Type::Top:
                    assert(false && "Top of the lattice has no parent!");
                default:
                    assert(false && "Unexpected lattice type at getParent()");
            }
        }
        static BasicLattice getLeastUpperBound(const BasicLattice & A,
                                               const BasicLattice & B) {
            if (A == B)
                return A;
            if (A.getRank() < B.getRank())
               return getLeastUpperBound(A, B.getParent()); 
            if (B.getRank() < A.getRank())
               return getLeastUpperBound(A.getParent(), B); 
            return getLeastUpperBound(A.getParent(), B.getParent()); 
        }
        static std::pair<BasicLattice, BasicLattice> toSameRank(
                                            const BasicLattice & A,
                                            const BasicLattice & B) {
            if (A.getRank() < B.getRank())
               return toSameRank(A, B.getParent()); 
            if (B.getRank() < A.getRank())
               return toSameRank(A.getParent(), B); 
            assert(A.getRank() == B.getRank());
            return {A, B};
        }

    };

    struct ConstantPropagation : public AbstractInterpretation<BasicLattice> {
        static char ID;

        ConstantPropagation() : AbstractInterpretation(ID) {}

        AbstractInterpretation::state_type flowState(const Instruction * inst,
                         AbstractInterpretation::state_type state) override {

            dumpState(state);
            errs() << "> INST: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';

            if (!isa<Value>(inst))
                return state;

            auto inst_name = inst->getName();

            switch (inst->getOpcode()) {
                case Instruction::Add: 
                {
                    auto lhs = inst->getOperand(0);
                    auto rhs = inst->getOperand(1);
                    auto lhs_value = BasicLattice();
                    auto rhs_value = BasicLattice();

                    // promote constants to SingleValue BasicLatticeType
                    // save variable names if any
                    if (auto lhs_c = dyn_cast<ConstantInt>(lhs)) {
                        lhs_value \
                            = BasicLattice(BasicLattice::Type::SingleValue,
                                           lhs_c->getSExtValue());
                    } else {
                        assert(!isa<Constant>(lhs));
                        lhs_value = state.at(lhs->getName()); 
                    }
                    if (auto rhs_c = dyn_cast<ConstantInt>(rhs)) {
                        rhs_value \
                            = BasicLattice(BasicLattice::Type::SingleValue,
                                           rhs_c->getSExtValue());
                    } else {
                        assert(!isa<Constant>(rhs) && "Add inst assert");
                        rhs_value = state.at(rhs->getName());
                    }

                    // toSameRank()
                    auto values = BasicLattice::toSameRank(lhs_value,
                                                           rhs_value);
                    lhs_value = values.first;
                    rhs_value = values.second;

                    // SingleValue -> add values
                    if (lhs_value.type == BasicLattice::Type::SingleValue &&
                        rhs_value.type == BasicLattice::Type::SingleValue) {
                        state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                        lhs_value.value + rhs_value.value);
                        return state; 
                    }
                    // Positive + (Positive || Zero) -> Positive
                    if (lhs_value.type == BasicLattice::Type::Positive &&
                        (rhs_value.type == BasicLattice::Type::Positive ||
                         rhs_value.type == BasicLattice::Type::Zero)) {
                        state[inst_name] = BasicLattice(BasicLattice::Type::Positive);
                        return state; 
                    }
                    // Negative -||-
                    if (lhs_value.type == BasicLattice::Type::Negative &&
                        (rhs_value.type == BasicLattice::Type::Negative ||
                         rhs_value.type == BasicLattice::Type::Zero)) {
                        state[inst_name] = BasicLattice(BasicLattice::Type::Negative);
                        return state; 
                    }
                    // Zero + Zero -> SingleValue of 0 !!!
                    if (lhs_value.type == BasicLattice::Type::Zero &&
                        rhs_value.type == BasicLattice::Type::Zero) {
                        state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                        0);
                        return state; 
                    }
                    // Positive + Negative -> Any
                    // Any -> Any
                    if ((lhs_value.type == BasicLattice::Type::Positive &&
                        rhs_value.type == BasicLattice::Type::Negative) ||
                        (lhs_value.type == BasicLattice::Type::Any &&
                        rhs_value.type == BasicLattice::Type::Any)) {

                        state[inst_name] = BasicLattice(BasicLattice::Type::Any);
                        return state; 
                    }
                    assert(false && "opAdd illegal combination");
                }
                case Instruction::Store: 
                {
                    auto lhs = inst->getOperand(0);
                    auto rhs = inst->getOperand(1);
                    auto lhs_value = BasicLattice();

                    // promote constants to SingleValue BasicLatticeType
                    // save variable names if any
                    if (auto lhs_c = dyn_cast<ConstantInt>(lhs)) {
                        lhs_value \
                            = BasicLattice(BasicLattice::Type::SingleValue,
                                           lhs_c->getSExtValue());
                    } else {
                        assert(!isa<Constant>(lhs) && "Store inst assert");
                        lhs_value = state.at(lhs->getName()); 
                    }
                    
                    state[rhs->getName()] = lhs_value;
                    return state;
                }
                case Instruction::Load: 
                {
                    auto lhs = inst->getOperand(0);
                    auto lhs_value = BasicLattice();

                    // promote constants to SingleValue BasicLatticeType
                    // save variable names if any
                    //assert(!isa<Constant>(lhs) && "Load inst assert");
                    lhs_value = state.at(lhs->getName()); 
                    
                    state[inst_name] = lhs_value;
                    return state;
                }
                default:
                    errs() << "Unknown opcode: ";
                    errs().write_escaped(inst->getOpcodeName()) << '\n';
                    return state;
            }
        }

        bool doInitialization(Function &F) {
            bool ret = AbstractInterpretation::doInitialization(F);
            // do stuff
            return ret;
        }

        bool doFinalization(Function &F) {
            // do stuff !!!
            return AbstractInterpretation::doFinalization(F);
        }
    };

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

char ConstantPropagation::ID = 3;
static RegisterPass<ConstantPropagation> X4("ai_cp", "ConstantPropagation Pass",
        false /* Only looks at CFG */,
        false /* Analysis Pass */);

