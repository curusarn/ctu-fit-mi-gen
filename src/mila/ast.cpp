#include "ast.h"

namespace mila {
namespace ast {

void Node::accept(Visitor * v) {
    v->visit(this);
}

Declaration::~Declaration() {
    delete value;
}


void Declaration::accept(Visitor * v) {
    v->visit(this);
}

void Arguments::accept(Visitor * v) {
    v->visit(this);
}

Function::~Function() {
    delete body;
}

void Function::accept(Visitor * v) {
    v->visit(this);
}

Functions::~Functions() {
    for (ast::Function * f : functions)
        delete f;
}


void Functions::accept(Visitor * v) {
    v->visit(this);
}


Declarations::~Declarations() {
    for (ast::Declaration * d : declarations)
        delete d;
}

void Declarations::accept(Visitor * v) {
    v->visit(this);
}


Module::~Module() {
    delete functions;
    delete declarations;
    delete body;
}

void Module::accept(Visitor * v) {
    v->visit(this);
}


Block::~Block() {
    for (Node * s : statements)
        delete s;
}

void Block::accept(Visitor * v) {
    v->visit(this);
}


Write::~Write() {
    delete expression;
}

void Write::accept(Visitor * v) {
    v->visit(this);
}

void Read::accept(Visitor * v) {
    v->visit(this);
}

If::~If() {
    delete condition;
    delete trueCase;
    delete falseCase;
}


void If::accept(Visitor * v) {
    v->visit(this);
}


While::~While() {
    delete condition;
    delete body;
}

void While::accept(Visitor * v) {
    v->visit(this);
}

Return::~Return() {
    delete value;
}


void Return::accept(Visitor * v) {
    v->visit(this);
}

Assignment::~Assignment() {
    delete value;
}


void Assignment::accept(Visitor * v) {
    v->visit(this);
}

void Expression::accept(Visitor * v) {
    v->visit(this);
}

Call::~Call() {
    for (Expression * a : arguments)
        delete a;
}

void Call::accept(Visitor * v) {
    v->visit(this);
}

Binary::~Binary() {
    delete lhs;
    delete rhs;
}


void Binary::accept(Visitor * v) {
    v->visit(this);
}

Unary::~Unary() {
    delete operand;
}

void Unary::accept(Visitor * v) {
    v->visit(this);
}

void Variable::accept(Visitor * v) {
    v->visit(this);
}

void Number::accept(Visitor * v) {
    v->visit(this);
}


}  
}
