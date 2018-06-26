#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
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
    static int isGreaterThan(BasicLattice::Type, BasicLattice::Type);
};

struct ConstantPropagation : public AbstractInterpretation<BasicLattice> {
    static char ID;
    std::map<const Instruction *, state_type> inst_state_in_;

    ConstantPropagation() : AbstractInterpretation(ID) {}

    bool postprocess(Function &F);
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
    void dumpState(const state_type & state);
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
    if (A.type == Type::Bottom)
        return B;
    if (B.type == Type::Bottom)
        return A;

    if (A == B)
        return A;
    if (A.getRank() < B.getRank())
       return getLeastUpperBound(A.getParent(), B); 
    if (B.getRank() < A.getRank())
       return getLeastUpperBound(A, B.getParent()); 
    return getLeastUpperBound(A.getParent(), B.getParent()); 
}

std::pair<BasicLattice, BasicLattice> BasicLattice::toSameRank(
                const std::pair<const BasicLattice, const BasicLattice> & ab) {
    return toSameRank(ab.first, ab.second);
}

std::pair<BasicLattice, BasicLattice> BasicLattice::toSameRank(
                                              const BasicLattice & A,
                                              const BasicLattice & B) {
    if (A.getRank() < B.getRank())
       return toSameRank(A.getParent(), B); 
    if (B.getRank() < A.getRank())
       return toSameRank(A, B.getParent()); 
    assert(A.getRank() == B.getRank());
    return {A, B};
}

// ConstantPropagation

void ConstantPropagation::dumpState(const state_type & state) {
    for (auto& x : state)
        errs() << x.first << ": "
               << static_cast<int>(x.second.type) << " ("
               << x.second.value << ")\n";
}

bool ConstantPropagation::postprocess(Function &F) {
    bool modified=false;  

    errs() << "--- POSTPROCESS ---\n";
    for (inst_iterator It = inst_begin(F), E = inst_end(F); It != E; ++It)
    {
        Instruction *inst = &*It;
        switch (inst->getOpcode()) {
        case Instruction::Store:
        case Instruction::PHI: 
        case Instruction::Load:
        case Instruction::Br:
        case Instruction::SExt:
            break; // SKIP

        case Instruction::ICmp:
        case Instruction::Ret:
        case Instruction::Call:
        case Instruction::Add:
        case Instruction::Sub:
        case Instruction::Mul:
        case Instruction::SDiv: {
            auto inst_name = inst->getName();
            errs() << "> INST: ";
            errs().write_escaped(inst_name) << '\n';

            auto inst_state = inst_state_in_.at(inst);
            dumpState(inst_state);
            auto num_operands = inst->getNumOperands();
            std::vector<const Value *> ops;
            assert(num_operands <= 3);
            for (unsigned i = 0; i < num_operands; ++i) {
                auto * op = inst->getOperand(i);
                auto op_name = op->getName();
                errs() << "> > op: ";
                errs().write_escaped(op_name) << '\n';
                if ("" != op_name &&
                    inst_state.count(op_name) &&
                    BasicLattice::Type::SingleValue == inst_state.at(op_name).type) {
                    
                    auto & context = inst->getContext();
                    auto val = inst_state.at(op_name).value;
                    Value * c = ConstantInt::get(context, llvm::APInt(32, val));
                    inst->setOperand(i, c);
                    // I'm deliberately leaking memory
                    errs() << "> > > replaced with: " << val << "\n";
                    modified = true;
                }
            }
        }
        } // end switch
    }
    inst_state_in_.clear();
    return modified;
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
    if (!state.count(val->getName())) {
        errs() << "[value2lattice produced Any]\n";
        return BasicLattice(BasicLattice::Type::Any);
    }
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

int BasicLattice::isGreaterThan(BasicLattice::Type at,
                                 BasicLattice::Type bt) {
    if ((at == BasicLattice::Type::Positive
         && 
         (bt == BasicLattice::Type::Zero
          ||
          bt == BasicLattice::Type::Negative)
        )
        ||
        (at == BasicLattice::Type::Zero
         &&
         bt == BasicLattice::Type::Negative)
       ) {
        return 1; 
    }
    if ((at == BasicLattice::Type::Negative
         ||
         at == BasicLattice::Type::Zero)
        && 
        (bt == BasicLattice::Type::Positive
         ||
         bt == BasicLattice::Type::Zero)
       ) {
        return 0; 
    }
    return -1;
}

ConstantPropagation::state_type ConstantPropagation::flowState(
                                    const Instruction * inst,
                                    ConstantPropagation::state_type state) {
    // save in states for every instruction for future processing
    inst_state_in_[inst] = state;
    
    //dumpState(state);
    //errs() << "> INST: ";
    //errs().write_escaped(inst->getOpcodeName()) << '\n';

    if (!isa<Value>(inst))
        return state;

    auto inst_name = inst->getName();
    auto num_operands = inst->getNumOperands();
    std::vector<const Value *> ops;
    assert(num_operands <= 3);
    for (unsigned i = 0; i < num_operands; ++i)
        ops.push_back(inst->getOperand(i));

    switch (inst->getOpcode()) {
    // SPECIAL INSTRUCTIONS
    case Instruction::Store: 
        state[ops[1]->getName()] = llvmValue2BasicLattice(ops[0], state);
        return state;
    case Instruction::PHI: {
        const auto & phi = cast<PHINode>(*inst);
        auto num_incoming = phi.getNumIncomingValues();
        auto lat = BasicLattice();
        for (unsigned i = 0; i < num_incoming; ++i) {
            BasicLattice tmp_lat = llvmValue2BasicLattice(
                                                    phi.getIncomingValue(i),
                                                    state);
            lat = BasicLattice::getLeastUpperBound(lat, tmp_lat);
        }
        state[inst_name] = lat;
        //state[inst_name] = BasicLattice(BasicLattice::Type::Any);
        return state;
    }


    // RETURN VALUE OF FIRST OPERAND 
    case Instruction::Load: 
    case Instruction::SExt:
        if (state.count(ops[0]->getName()))
            state[inst_name] = state.at(ops[0]->getName());
        else
            state[inst_name] = BasicLattice(BasicLattice::Type::Any);
        return state;

    // RETURN BasicLattice::Type::Any
    case Instruction::Call: 
        if ("" != inst_name)
            state[inst_name] = BasicLattice(BasicLattice::Type::Any);
        return state;

    // DO NOTHING
    case Instruction::Ret: 
    case Instruction::Br: 
        // do nothing?
        return state;

    // 2 OPERAND INSTRUCTIONS
    default:
        // all other instructions have two operands
        assert(num_operands == 2 && inst->getOpcodeName());
        auto x = llvmValue2BasicLattice({ops[0], ops[1]}, state);
        x = BasicLattice::toSameRank(x);
        auto a = x.first;
        auto b = x.second;
        auto at = a.type;
        auto bt = b.type;
        int rank = a.getRank();
    
        // NESTED SWITCH for rest of the instructions
        switch (inst->getOpcode()) {
        case Instruction::ICmp: {
            const auto & cmp = cast<ICmpInst>(*inst);
            // both are SingleValue
            if (at == BasicLattice::Type::SingleValue
                && bt == BasicLattice::Type::SingleValue) {
                int res;
                switch (cmp.getPredicate()) {
                case ICmpInst::ICMP_EQ:
                    res = (a.value == b.value);
                    break;
                case ICmpInst::ICMP_NE:
                    res = (a.value != b.value);
                    break;
                case ICmpInst::ICMP_SGT:
                    res = (a.value > b.value);
                    break;
                case ICmpInst::ICMP_SGE:
                    res = (a.value >= b.value);
                    break;
                case ICmpInst::ICMP_SLT:
                    res = (a.value < b.value);
                    break;
                case ICmpInst::ICMP_SLE:
                    res = (a.value <= b.value);
                    break;
                default:
                    assert(false && "ICmp-SingleValue illegal predicate");
                }
                state[inst_name] = 
                    BasicLattice(BasicLattice::Type::SingleValue, res);
                return state;
            }
            if (2 == rank) {
                int res;
                switch (cmp.getPredicate()) {
                case ICmpInst::ICMP_EQ:
                    break;
                case ICmpInst::ICMP_NE:
                    if (at != bt) {
                        state[inst_name] = 
                            BasicLattice(BasicLattice::Type::SingleValue, 1);
                        return state; 
                    }
                    break;
                case ICmpInst::ICMP_SGT:
                    res = BasicLattice::isGreaterThan(at, bt);
                    if (-1 == res) 
                        break;
                    state[inst_name] =
                            BasicLattice(BasicLattice::Type::SingleValue, res);
                    return state;
                case ICmpInst::ICMP_SGE:
                    res = BasicLattice::isGreaterThan(at, bt);
                    if (at == BasicLattice::Type::Zero
                        && bt == BasicLattice::Type::Zero)
                        res = 1;
                    if (-1 == res) 
                        break;
                    state[inst_name] =
                            BasicLattice(BasicLattice::Type::SingleValue, res);
                    return state;
                case ICmpInst::ICMP_SLT:
                    res = ! BasicLattice::isGreaterThan(at, bt);
                    if (at == BasicLattice::Type::Zero
                        && bt == BasicLattice::Type::Zero)
                        res = 0;
                    if (-1 == res) 
                        break;
                    state[inst_name] =
                            BasicLattice(BasicLattice::Type::SingleValue, res);
                    return state;
                case ICmpInst::ICMP_SLE:
                    res = ! BasicLattice::isGreaterThan(at, bt);
                    if (-1 == res) 
                        break;
                    state[inst_name] =
                            BasicLattice(BasicLattice::Type::SingleValue, res);
                    return state;
                default:
                    assert(false && "ICmp-SingleValue illegal predicate");
                }
            }
            state[inst_name] = BasicLattice(BasicLattice::Type::Any);
            return state;
        }
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
            // Zero + Positive -> Positive
            if ((at == BasicLattice::Type::Positive
                 &&
                 (bt == BasicLattice::Type::Positive
                  ||
                  bt == BasicLattice::Type::Zero)
                )
                ||
                (at == BasicLattice::Type::Zero
                 &&
                 bt == BasicLattice::Type::Positive 
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
                (at == BasicLattice::Type::Zero
                 &&
                 bt == BasicLattice::Type::Negative 
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Negative);
                return state; 
            }
            // Zero + Zero -> SingleValue of 0 !!!
            if (at == BasicLattice::Type::Zero &&
                bt == BasicLattice::Type::Zero) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                0);
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
        case Instruction::Sub: 
            // SingleValue -> sub values
            if (at == BasicLattice::Type::SingleValue
                &&
                bt == BasicLattice::Type::SingleValue) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                a.value - b.value);
                return state; 
            }
            // Positive - (Negative || Zero) -> Positive
            // Zero - Negative -> Positive
            if ((at == BasicLattice::Type::Positive
                 &&
                 (bt == BasicLattice::Type::Negative
                  ||
                  bt == BasicLattice::Type::Zero)
                )
                ||
                (at == BasicLattice::Type::Zero
                 &&
                 bt == BasicLattice::Type::Negative
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Positive);
                return state; 
            }
            // Negative - (Positive || Zero) -> Negative
            // Zero - Positive -> Negative
            if ((at == BasicLattice::Type::Negative
                 &&
                 (bt == BasicLattice::Type::Positive
                  ||
                  bt == BasicLattice::Type::Zero)
                )
                ||
                (at == BasicLattice::Type::Zero
                 &&
                 bt == BasicLattice::Type::Positive
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Negative);
                return state; 
            }
            // Zero + Zero -> SingleValue of 0 !!!
            if (at == BasicLattice::Type::Zero &&
                bt == BasicLattice::Type::Zero) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                0);
                return state; 
            }
            // Any -> Any
            if ((at == BasicLattice::Type::Positive
                 &&
                 bt == BasicLattice::Type::Positive)
                ||
                (bt == BasicLattice::Type::Negative
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
            assert(false && "opSub illegal combination");
        case Instruction::Mul: 
            // SingleValue -> mul values
            if (at == BasicLattice::Type::SingleValue
                &&
                bt == BasicLattice::Type::SingleValue) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                a.value * b.value);
                return state; 
            }
            // Positive * Positive -> Positive
            // Negative * Negative -> Positive
            if ((at == BasicLattice::Type::Positive
                 &&
                 bt == BasicLattice::Type::Positive
                )
                ||
                (at == BasicLattice::Type::Negative
                 &&
                 bt == BasicLattice::Type::Negative
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Positive);
                return state; 
            }
            // Positive * Negative -> Negative
            // Negative * Positive -> Negative
            if ((at == BasicLattice::Type::Positive
                 &&
                 bt == BasicLattice::Type::Negative
                )
                ||
                (at == BasicLattice::Type::Negative
                 &&
                 bt == BasicLattice::Type::Positive
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Negative);
                return state; 
            }
            // Zero * Zero -> SingleValue of 0 !!!
            if (at == BasicLattice::Type::Zero || 
                bt == BasicLattice::Type::Zero) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                0);
                return state; 
            }
            // Any -> Any
            if (at == BasicLattice::Type::Any
                &&
                bt == BasicLattice::Type::Any
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Any);
                return state; 
            }
            assert(false && "opMul illegal combination");
        case Instruction::SDiv: 
            // SingleValue -> div values
            if (at == BasicLattice::Type::SingleValue
                &&
                bt == BasicLattice::Type::SingleValue) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                a.value / b.value);
                return state; 
            }
            // Positive / Positive -> Positive
            // Negative / Negative -> Positive
            if ((at == BasicLattice::Type::Positive
                 &&
                 bt == BasicLattice::Type::Positive
                )
                ||
                (at == BasicLattice::Type::Negative
                 &&
                 bt == BasicLattice::Type::Negative
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Positive);
                return state; 
            }
            // Positive / Negative -> Negative
            // Negative / Positive -> Negative
            if ((at == BasicLattice::Type::Positive
                 &&
                 bt == BasicLattice::Type::Negative
                )
                ||
                (at == BasicLattice::Type::Negative
                 &&
                 bt == BasicLattice::Type::Positive
                )
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Negative);
                return state; 
            }
            // Zero / X -> SingleValue of 0 !!!
            // this could work better
            if (at == BasicLattice::Type::Zero) {
                state[inst_name] = BasicLattice(BasicLattice::Type::SingleValue,
                                                0);
                return state; 
            }
            // X / Zero -> assert
            if (bt == BasicLattice::Type::Zero) {
                assert(false && "can't divide by zero");
            }
            // Any -> Any
            if (at == BasicLattice::Type::Any
                &&
                bt == BasicLattice::Type::Any
               ) {
                state[inst_name] = BasicLattice(BasicLattice::Type::Any);
                return state; 
            }
            assert(false && "opDiv illegal combination");
        default:
            errs() << "Unknown opcode: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';
            errs() << "Unknown opcode: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';
            errs() << "Unknown opcode: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';
            errs() << "Unknown opcode: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';
            errs() << "Unknown opcode: ";
            errs().write_escaped(inst->getOpcodeName()) << '\n';
            assert(false);
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

