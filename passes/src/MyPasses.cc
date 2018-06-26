#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include <iostream>
#include "AbstractInterpretation.h"

using namespace llvm;

namespace my_passes {

struct Hello : public FunctionPass {
    static char ID;
    Hello() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
        errs() << "Hello: ";
        errs().write_escaped(F.getName()) << '\n';
        return false;
    }
}; // end of struct Hello

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
    explicit BasicLattice(Type t, int v = 0) : value(v), type(t) {};

    static BasicLattice getTop() { return BasicLattice(Type::Top); }
    static BasicLattice getBottom() { return BasicLattice(Type::Bottom); }

    bool operator== (const BasicLattice & B) const;

    BasicLattice getParent() const;
    static BasicLattice getLeastUpperBound(const BasicLattice & A,
                                           const BasicLattice & B); 
    static std::pair<BasicLattice, BasicLattice> toSameRank(
                const std::pair<const BasicLattice, const BasicLattice> & ab);
    static std::pair<BasicLattice, BasicLattice> toSameRank(
                                           const BasicLattice & A,
                                           const BasicLattice & B);
};

struct ConstantPropagation : public AbstractInterpretation<BasicLattice> {
    static char ID;
    std::map<const Instruction *, state_type> inst_state_in_;

    ConstantPropagation() : AbstractInterpretation(ID) {}

    bool doInitialization(Function &F);
    bool doFinalization(Function &F);
    ConstantPropagation::state_type getEntryBlockState(
                                            const BasicBlock & bb) override;
    ConstantPropagation::state_type flowState(
                            const Instruction * inst,
                            ConstantPropagation::state_type state) override;
    private:
    std::pair<BasicLattice, BasicLattice> llvmValue2BasicLattice(
                                std::pair<const Value *, const Value *> vals,
                                const state_type & state) const;
    BasicLattice llvmValue2BasicLattice(const Value * val,
                                        const state_type & state) const;
};

// BasicLattice 

bool BasicLattice::operator== (const BasicLattice & B) const {
    if (type == B.type) {
        if (Type::SingleValue == type)
            return value == B.value;
        return true;
    }

    return false;
}

BasicLattice BasicLattice::getParent() const {
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

BasicLattice BasicLattice::getLeastUpperBound(const BasicLattice & A,
                                              const BasicLattice & B) {
    if (A == B)
        return A;
    if (A.getRank() < B.getRank())
       return getLeastUpperBound(A, B.getParent()); 
    if (B.getRank() < A.getRank())
       return getLeastUpperBound(A.getParent(), B); 
    return getLeastUpperBound(A.getParent(), B.getParent()); 
}

std::pair<BasicLattice, BasicLattice> BasicLattice::toSameRank(
                const std::pair<const BasicLattice, const BasicLattice> & ab) {
    const auto & A = ab.first;
    const auto & B = ab.second;
    if (A.getRank() < B.getRank())
       return toSameRank(A, B.getParent()); 
    if (B.getRank() < A.getRank())
       return toSameRank(A.getParent(), B); 
    assert(A.getRank() == B.getRank());
    return {A, B};
}

// ConstantPropagation

bool ConstantPropagation::doInitialization(Function &F) {
    bool ret = AbstractInterpretation::doInitialization(F);
    // do stuff
    return ret;
}

bool ConstantPropagation::doFinalization(Function &F) {
    // do stuff !?!
    return AbstractInterpretation::doFinalization(F);
}

std::pair<BasicLattice, BasicLattice>
                        ConstantPropagation::llvmValue2BasicLattice(
                                std::pair<const Value *, const Value *> vals,
                                const state_type & state) const {
    return {llvmValue2BasicLattice(vals.first, state),
            llvmValue2BasicLattice(vals.second, state)};
}

BasicLattice ConstantPropagation::llvmValue2BasicLattice(
                                    const Value * val,
                                    const state_type & state) const {
    if (auto constant = dyn_cast<ConstantInt>(val))
        return BasicLattice(BasicLattice::Type::SingleValue,
                            constant->getSExtValue());
    assert(!isa<Constant>(val));
    return state.at(val->getName()); 
}

ConstantPropagation::state_type ConstantPropagation::getEntryBlockState(
                                    const BasicBlock & bb) {
    auto state = state_type();
    const Function * f = bb.getParent();
    // set all incoming arg to BasicLattice::Type::Any
    for (auto it = f->arg_begin(); it != f->arg_end(); ++it) 
        state[it->getName()] = BasicLattice(BasicLattice::Type::Any);
    return state_type();
}

ConstantPropagation::state_type ConstantPropagation::flowState(
                                    const Instruction * inst,
                                    ConstantPropagation::state_type state) {
    // save in states for every instruction for future processing
    inst_state_in_[inst] = state;
    
    dumpState(state);
    errs() << "> INST: ";
    errs().write_escaped(inst->getOpcodeName()) << '\n';
    errs() << "> INST: ";
    errs().write_escaped(inst->getOpcodeName()) << '\n';
    errs() << "> INST: ";
    errs().write_escaped(inst->getOpcodeName()) << '\n';

    if (!isa<Value>(inst))
        return state;

    auto inst_name = inst->getName();
    auto operand_count = inst->getNumOperands();
    std::vector<const Value *> ops;
    assert(operand_count <= 2);
    for (unsigned i = 0; i < operand_count; ++i)
        ops.push_back(inst->getOperand(i));

    switch (inst->getOpcode()) {
    case Instruction::Load: 
        state[inst_name] = state.at(ops[0]->getName());
        return state;
    case Instruction::Store: 
        state[ops[1]->getName()] = llvmValue2BasicLattice(ops[0], state);
        return state;
    case Instruction::Ret: 
        // do nothing?
        return state;
    default:
        // all other instructions have two operands
        assert(operand_count == 2 && inst->getOpcodeName());
        auto x = llvmValue2BasicLattice({ops[0], ops[1]}, state);
        x = BasicLattice::toSameRank(x);
        auto a = x.first;
        auto b = x.second;
        auto at = a.type;
        auto bt = b.type;
    
        // NESTED SWITCH for rest of the instructions
        switch (inst->getOpcode()) {
        case Instruction::Add: 
            // SingleValue -> add values
            if (at == BasicLattice::Type::SingleValue
                &&
                bt == BasicLattice::Type::SingleValue) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                a.value + b.value);
                return state; 
            }
            // Positive + (Positive || Zero) -> Positive
            if ((at == BasicLattice::Type::Positive
                 &&
                 (bt == BasicLattice::Type::Positive
                  ||
                  bt == BasicLattice::Type::Zero)
                )
                ||
                (bt == BasicLattice::Type::Positive
                 &&
                 (at == BasicLattice::Type::Positive 
                  ||
                  at == BasicLattice::Type::Zero)
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Positive);
                return state; 
            }
            // Negative -||-
            if ((at == BasicLattice::Type::Negative
                 &&
                 (bt == BasicLattice::Type::Negative
                  ||
                  bt == BasicLattice::Type::Zero)
                )
                ||
                (bt == BasicLattice::Type::Negative
                 &&
                 (at == BasicLattice::Type::Negative 
                  ||
                  at == BasicLattice::Type::Zero)
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Negative);
                return state; 
            }
            // Zero + Zero -> SingleValue of 0 !!!
            if (at == BasicLattice::Type::Zero &&
                bt == BasicLattice::Type::Zero) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue, 0);
                return state; 
            }
            // Positive + Negative -> Any
            // Any -> Any
            if ((at == BasicLattice::Type::Positive
                 &&
                 bt == BasicLattice::Type::Negative)
                ||
                (bt == BasicLattice::Type::Positive
                 &&
                 at == BasicLattice::Type::Negative)
                ||
                (at == BasicLattice::Type::Any
                 &&
                 bt == BasicLattice::Type::Any)
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Any);
                return state; 
            }
            assert(false && "opAdd illegal combination");
        default:
            errs() << "Unknown opcode: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';
            return state;
        } // end nested switch
    } // end top level switch
}

}  // end of my_passes namespace

using namespace my_passes;


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

