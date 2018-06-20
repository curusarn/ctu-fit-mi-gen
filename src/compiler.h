#ifndef COMPILER_H
#define COMPILER_H

#include "llvm.h"

#include "mila/ast.h"

namespace mila {

class CompilerError : public Exception {
public:
    CompilerError(std::string const & what, ast::Node const * ast):
        Exception(STR(what << " (line: " << ast->line << ", col: " << ast->col << ")")) {
    }
    CompilerError(std::string const & what):
        Exception(what) {
    }

};

/** Compiler */
class Compiler: public ast::Visitor {
public:
    static llvm::Function * compile(ast::Module * module) {
        Compiler c;
        module->accept(&c);

        // check that the module's IR is well formed
        llvm::raw_os_ostream err(std::cerr);
        if (llvm::verifyModule(*c.m, & err)) {
            c.m->print(llvm::outs(), nullptr);
            throw CompilerError("Invalid LLVM bitcode produced");
        }

        // return the main function
        return c.f;
    }

protected:
    virtual void visit(ast::Node * n) {
        throw Exception("Unknown compiler handler");
    }

    virtual void visit(ast::Expression * d) {
        throw Exception("Unknown compiler handler");
    }

    void compileDeclarations(ast::Declarations * ds, bool isGlobal = false) {
        for (ast::Declaration * d : ds->declarations) {
            if (c->hasVariable(d->symbol))
                throw CompilerError(STR("Redefinition of variable " << d->symbol), d);
            if (d->value == nullptr) {
                if (isGlobal) {
                    auto gv = new llvm::GlobalVariable(*m, t_int, false, llvm::GlobalValue::CommonLinkage, nullptr, d->symbol.name() + "_");
                    gv->setAlignment(4);
                    gv->setInitializer(llvm::ConstantInt::get(context, llvm::APInt(32, 0)));
                    c->variables[d->symbol] = Location::variable(gv);
                }
                else
                    c->variables[d->symbol] = Location::variable(new llvm::AllocaInst(t_int, 0, d->symbol.name(), bb));
            } else  {
                d->value->accept(this);
                c->variables[d->symbol] = Location::constant(result);
            }
        }
    }

    virtual void visit(ast::Declaration * d) {
        throw Exception("This should be unreachable");
    }

    virtual void visit(ast::Function * f) {
        // main is the name of implicit function
        if (f->name == "main")
            throw CompilerError("Cannot create user defined main function", f);
        // first create the function type
        std::vector<llvm::Type * > at;
        for (size_t i = 0, e = f->arguments.size(); i != e; ++i)
            at.push_back(t_int);
        llvm::FunctionType * ft = llvm::FunctionType::get(t_int, at, false);
        // now create the function
        if (m->getFunction(f->name.name()) != nullptr)
            throw CompilerError(STR("Function " << f->name << " already exists"), f);
        this->f = llvm::Function::Create(ft, llvm::GlobalValue::ExternalLinkage, f->name.name(), m);
        this->f->setCallingConv(llvm::CallingConv::C);
        // create the context for the function block
        c = new BlockContext(c);
        // create the initial basic block for the function
        bb = llvm::BasicBlock::Create(context, "", this->f);
        // arguments to the function are in registers, create variables for them 
        llvm::Function::arg_iterator args = this->f->arg_begin();
        for (Symbol const & s: f->arguments) {
            llvm::Value * v = args++;
            if (c->hasVariable(s))
                throw CompilerError(STR("Redefinition of variable " << s), f);
            llvm::AllocaInst * loc = new llvm::AllocaInst(t_int, 0, s.name(), bb);
            c->variables[s] = Location::variable(loc);
            new llvm::StoreInst(v, loc, false, bb);
            // set names, just for debugging purposes
            v->setName(s.name());
            loc->setName(s.name());
        }
        // compile the body of the function
        compileFunctionBody(f->body);
        // unroll the block context
        BlockContext * x = c;
        c = c->parent;
        delete x;
    }

    void compileFunctionBody(ast::Node * node) {
        // now compile the body
        node->accept(this);

        // don't insert return statemaent if there is one already 
        if (bb == nullptr)
            return;
        // finally, check if last instruction was not return, and if not emit return 0
        if (result == nullptr)
            result = llvm::ReturnInst::Create(context, zero, bb);
        else if (not llvm::isa<llvm::ReturnInst>(result))
            result = llvm::ReturnInst::Create(context, result, bb);
    }

    virtual void visit(ast::Functions * fs) {
        for (ast::Function * f : fs->functions)
            f->accept(this);
    }

    virtual void visit(ast::Declarations * ds) {
        compileDeclarations(ds);
    }

    virtual void visit(ast::Module * module) {
        result = nullptr;
        // create the module
        m = new llvm::Module("mila", context);
        // add declarations for runtime functions (read and write), first types
        // then create the functions
        llvm::Function::Create(t_read, llvm::GlobalValue::ExternalLinkage, "read_", m)->setCallingConv(llvm::CallingConv::C);
        llvm::Function::Create(t_write, llvm::GlobalValue::ExternalLinkage, "write_", m)->setCallingConv(llvm::CallingConv::C);

        // start context for globals
        c = new BlockContext(nullptr); // globals
        compileDeclarations(module->declarations, true);
        // compile all functions
        module->functions->accept(this);
        // now create main function, and compile the pre-block declarations
        llvm::FunctionType * ft = llvm::FunctionType::get(t_int, false);
        f = llvm::Function::Create(ft, llvm::GlobalValue::ExternalLinkage, "main", m);
        // create the initial basic block and compile the body
        bb = llvm::BasicBlock::Create(context, "", this->f);
        compileFunctionBody(module->body);
    }

    virtual void visit(ast::Block * d) {
        // create the context for the function block
        c = new BlockContext(c);
        // compile declarations
        d->declarations->accept(this);
        // and all statements in the block
        for (ast::Node * s : d->statements) {
            if (bb == nullptr)
                throw CompilerError("Code after return statement is not allowed", s);
            s->accept(this);
        }
        // unroll the block context
        BlockContext * x = c;
        c = c->parent;
        delete x;
    }

    virtual void visit(ast::Write * w) {
        w->expression->accept(this);
        llvm::CallInst::Create(m->getFunction("write_"), result, "", bb);
        // result of write is the expression it wrote
    }

    virtual void visit(ast::Read * r) {
        result = llvm::CallInst::Create(m->getFunction("read_"), r->symbol.name(), bb);
        Location const & l = c->get(r->symbol, r);
        if (l.isConstant())
            throw CompilerError(STR("Cannot assign constant " << r->symbol), r);
        new llvm::StoreInst(result, l.address(), false, bb);
        // keep read value in result
    }

    virtual void visit(ast::If * s) {
        // compile the condition
        s->condition->accept(this);
        // create basic blocks for the true & false cases and continuation
        llvm::BasicBlock * trueCase = llvm::BasicBlock::Create(context, "trueCase", f);
        llvm::BasicBlock * falseCase = llvm::BasicBlock::Create(context, "falseCase", f);
        llvm::BasicBlock * next = llvm::BasicBlock::Create(context, "next", f);

        llvm::ICmpInst * cmp = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_NE, result, zero, "if_cond");
        llvm::BranchInst::Create(trueCase, falseCase, cmp, bb);

        bb = trueCase;
        s->trueCase->accept(this);
        trueCase = bb;
        llvm::Value * trueResult = result;
        if (trueCase != nullptr)
            llvm::BranchInst::Create(next, bb);
        bb = falseCase;
        s->falseCase->accept(this);
        falseCase = bb;
        llvm::Value * falseResult = result;
        if (falseCase != nullptr)
            llvm::BranchInst::Create(next, bb);
        bb = next;
        if (falseCase == nullptr and trueCase == nullptr) {
            next->eraseFromParent();
            result = nullptr;
            bb = nullptr;
        } else {
            llvm::PHINode * phi = llvm::PHINode::Create(t_int, 2, "if_phi", bb);
            if (trueCase != nullptr)
                phi->addIncoming(trueResult, trueCase);
            if (falseCase != nullptr)
                phi->addIncoming(falseResult, falseCase);
            result = phi;
        }
    }

    virtual void visit(ast::While * d) {
        // homework

        // save prev bb
        llvm::Value * prevResult = result;
        llvm::BasicBlock * prevBB = bb;

        // create basic blocks for condition, cycle body and continuation
        llvm::BasicBlock * condition = llvm::BasicBlock::Create(context, "condition", f);
        llvm::BasicBlock * cycleBody = llvm::BasicBlock::Create(context, "cycleBody", f);
        llvm::BasicBlock * next = llvm::BasicBlock::Create(context, "next", f);

        // jump to condition block
        llvm::BranchInst::Create(condition, bb);
        bb = condition;


        // create phi node
        llvm::PHINode * phi = llvm::PHINode::Create(t_int, 2, "while_phi", bb);
        phi->addIncoming(prevResult, prevBB);

        // compile the condition
        d->condition->accept(this);

        llvm::ICmpInst * cmp = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_NE, result, zero, "while_cond");
        llvm::BranchInst::Create(cycleBody, next, cmp, bb);

        bb = cycleBody;
        d->body->accept(this);
        cycleBody = bb;
        llvm::Value * trueResult = result;

        if (cycleBody != nullptr) {
            llvm::BranchInst::Create(condition, bb);
            phi->addIncoming(trueResult, cycleBody);
        } else
            phi->eraseFromParent();

        result = zero;
        bb = next;
    }

    virtual void visit(ast::Return * r) {
        // homework
        r->value->accept(this);
        result = llvm::ReturnInst::Create(context, result, bb);
        if (result == nullptr)
            throw std::exception();
        bb = nullptr;
    }

    virtual void visit(ast::Assignment * a) {
        a->value->accept(this);
        // now get the variable and store the value in it
        Location const & l = c->get(a->symbol, a);
        if (l.isConstant())
            throw CompilerError(STR("Cannot assign constant " << a->symbol), a);
        new llvm::StoreInst(result, l.address(), false, bb);
        // keep the stored value in result
    }

    virtual void visit(ast::Call * call) {
        std::vector<llvm::Value * > args;
        for (ast::Node * a : call->arguments) {
            a->accept(this);
            args.push_back(result);
        }
        llvm::Function * f = m->getFunction(call->function.name());
        if (f == nullptr)
            throw CompilerError(STR("Call to undefined function " << call->function), call);
        if (f->arg_size() != args.size())
            throw CompilerError(STR("Function " << call->function << " declared with different number of arguments"), call);
        result = llvm::CallInst::Create(f, args, call->function.name(), bb);
    }

    virtual void visit(ast::Binary * op) {
        // homework
        op->lhs->accept(this);
        llvm::Value * lhsValue = result;
        op->rhs->accept(this);

        switch (op->type) {
            case Token::Type::opAdd:
                result = llvm::BinaryOperator::Create(llvm::Instruction::Add, lhsValue, result, "add", bb);
                break;
            case Token::Type::opSub:
                result = llvm::BinaryOperator::Create(llvm::Instruction::Sub, lhsValue, result, "sub", bb);
                break;
            case Token::Type::opMul:
                result = llvm::BinaryOperator::Create(llvm::Instruction::Mul, lhsValue, result, "mul", bb);
                break;
            case Token::Type::opDiv:
                result = llvm::BinaryOperator::Create(llvm::Instruction::SDiv, lhsValue, result, "sdiv", bb);
                break;
            case Token::Type::opEq:
                result = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_EQ, lhsValue, result, "eq");
                result = llvm::CastInst::Create(llvm::Instruction::SExt, result, llvm::Type::getInt32Ty(context), "i32", bb);
                break;
            case Token::Type::opNeq:
                result = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_NE, lhsValue, result, "ne");
                result = llvm::CastInst::Create(llvm::Instruction::SExt, result, llvm::Type::getInt32Ty(context), "i32", bb);
                break;
            case Token::Type::opLt:
                result = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_SLT, lhsValue, result, "slt");
                result = llvm::CastInst::Create(llvm::Instruction::SExt, result, llvm::Type::getInt32Ty(context), "i32", bb);
                break;
            case Token::Type::opGt:
                result = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_SGT, lhsValue, result, "sgt");
                result = llvm::CastInst::Create(llvm::Instruction::SExt, result, llvm::Type::getInt32Ty(context), "i32", bb);
                break;
            case Token::Type::opLte:
                result = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_SLE, lhsValue, result, "sle");
                result = llvm::CastInst::Create(llvm::Instruction::SExt, result, llvm::Type::getInt32Ty(context), "i32", bb);
                break;
            case Token::Type::opGte:
                result = new llvm::ICmpInst(*bb, llvm::ICmpInst::ICMP_SGE, lhsValue, result, "sge");
                result = llvm::CastInst::Create(llvm::Instruction::SExt, result, llvm::Type::getInt32Ty(context), "i32", bb);
                break;
            default:
                throw Exception("Unknown binary operator token type");
        }
    }

    virtual void visit(ast::Unary * op) {
        // homework
        op->operand->accept(this);

        switch (op->type) {
            case Token::Type::opAdd:
                break;
            case Token::Type::opSub:
                result = llvm::BinaryOperator::Create(llvm::Instruction::Sub, zero, result, "neg", bb);
                break;
            default:
                throw Exception("Unknown unary operator token type");
        }
    }

    virtual void visit(ast::Variable * v) {
        Location const & l = c->get(v->symbol, v);
        if (l.isConstant())
            result = l.value();
        else
            result = new llvm::LoadInst(l.address(), v->symbol, bb);
    }

    virtual void visit(ast::Number * n) {
        result = llvm::ConstantInt::get(context, llvm::APInt(32, n->value));
    }

private:

    class Location {
    public:
        static Location variable(llvm::Value * address) {
            return Location(address, false);
        }

        static Location constant(llvm::Value * value) {
            return Location(value, true);
        }

        bool isConstant() const {
            return isConstant_;
        }

        llvm::Value * value() const {
            assert (isConstant_);
            return location_;
        }

        llvm::Value * address() const {
            assert (not isConstant_);
            return location_;
        }

        Location(Location const & ) = default;

        Location & operator = (Location const &) = default;

        Location():
            location_(nullptr),
            isConstant_(false) {
        }

        Location(llvm::Value * location, bool isConstant):
            location_(location),
            isConstant_(isConstant) {
        }

    private:

        llvm::Value * location_;
        bool isConstant_;
    };

    /** Compilation context for a block.


     */
    class BlockContext {
    public:

        bool hasVariable(Symbol symbol) const {
            return variables.find(symbol) != variables.end();
        }

        Location const & get(Symbol symbol, ast::Node * ast) {
            auto i = variables.find(symbol);
            if (i != variables.end())
                return i->second;
            if (parent != nullptr)
                return parent->get(symbol, ast);
            throw CompilerError(STR("Variable or constant " << symbol << " not found"), ast);
        }

        std::map<Symbol, Location> variables;

        BlockContext * parent;



        BlockContext(BlockContext * parent = nullptr):
            parent(parent) {
        }

    };

    llvm::Module * m;

    llvm::Function * f;

    llvm::BasicBlock * bb;

    BlockContext * c;

    llvm::Value * result;



    static llvm::Type * t_int;
    static llvm::Type * t_void;

    static llvm::FunctionType * t_read;
    static llvm::FunctionType * t_write;


    static llvm::Value * zero;
    static llvm::Value * one;

    static llvm::LLVMContext & context;

};
  
}

#endif
