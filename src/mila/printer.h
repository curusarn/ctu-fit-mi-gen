#ifndef MILA_PRINTER_H
#define MILA_PRINTER_H

#include "ast.h"

namespace mila {
namespace ast {

class Printer : public ast::Visitor {
public:
    static void print(Node * n) {
        print(n, std::cout);
    }

    static void print(Node * n, std::ostream & stream) {
        Printer p(stream);
        n->accept(&p);
    }

protected:

    Printer(std::ostream & stream):
        stream(stream) {
    }

    void visit(Node * n) {
        stream << "!!!";
    }

    void visit(Declaration * d) {
        if (d->value == nullptr) {
            stream << "var " << d->symbol << std::endl;
        } else {
            stream << "const " << d->symbol << " = ";
            d->value->accept(this);
            stream << std::endl;
        }
    }

    void visit(Function * f) {
        stream << "function " << f->name;
        stream << "(";
        if (not f->arguments.empty()) {
            stream << f->arguments[0];
            for (size_t i = 1, e = f->arguments.size(); i < e; ++i)
                stream << ", " << f->arguments[i];
        }
        stream << ") ";
        f->body->accept(this);
        stream << std::endl;
    }

    void visit(Functions * fs) {
        for (Function * f : fs->functions)
            f->accept(this);
    }

    void visit(Declarations * ds) {
        for (ast::Declaration * d : ds->declarations)
            d->accept(this);
    }

    void visit(Module * m) {
        m->functions->accept(this);
        m->declarations->accept(this);
        m->body->accept(this);
    }

    void visit(Block * b) {
        stream << "begin" << std::endl;
        for (Node * s : b->statements) {
            stream << "    ";
            s->accept(this);
            stream << std::endl;
        }
        stream <<  "end" << std::endl;
    }

    void visit(Write * w) {
        stream << "write ";
        w->expression->accept(this);
    }

    void visit(Read * r) {
        stream << "read " << r->symbol;
    }

    void visit(If * s) {
        stream << "if ";
        s->condition->accept(this);
        stream << " then ";
        s->trueCase->accept(this);
        stream << " else ";
        s->falseCase->accept(this);
    }

    void visit(While * s) {
        stream << "while ";
        s->condition->accept(this);
        stream << " do ";
        s->body->accept(this);
    }

    void visit(Return * s) {
        stream << "return ";
        if (s->value != nullptr)
            s->value->accept(this);
    }

    void visit(Assignment * a) {
        stream << a->symbol << " := ";
        a->value->accept(this);
    }

    void visit(Call * c) {
        stream << c->function << "(";
        if (not c->arguments.empty()) {
            c->arguments[0]->accept(this);
            for (size_t i = 1, e = c->arguments.size(); i < e; ++i) {
                stream << ", ";
                c->arguments[i]->accept(this);
            }
        }
        stream << ")";
    }

    void visit(Binary * b) {
        b->lhs->accept(this);
        switch (b->type) {
            case Token::Type::opAdd:
                stream << " + ";
                break;
            case Token::Type::opSub:
                stream << " - ";
                break;
            case Token::Type::opMul:
                stream << " * ";
                break;
            case Token::Type::opDiv:
                stream << " / ";
                break;
            case Token::Type::opEq:
                stream << " = ";
                break;
            case Token::Type::opNeq:
                stream << " <> ";
                break;
            case Token::Type::opLt:
                stream << " < ";
                break;
            case Token::Type::opGt:
                stream << " > ";
                break;
            case Token::Type::opLte:
                stream << " <= ";
                break;
            case Token::Type::opGte:
                stream << " >= ";
                break;
            default:
                stream << " !" << Token::typeToString(b->type) << "! ";
        }
        b->rhs->accept(this);
    }

    void visit(Unary * u) {
        switch (u->type) {
            case Token::Type::opAdd:
                stream << " + ";
                break;
            case Token::Type::opSub:
                stream << " - ";
                break;
            default:
                stream << " !" << Token::typeToString(u->type) << "! ";
        }
        u->operand->accept(this);
    }

    void visit(Variable * v ) {
        stream << v->symbol;
    }

    void visit(Number * n) {
        stream << n->value;
    }

private:
    std::ostream & stream;


};


}
}

#endif
