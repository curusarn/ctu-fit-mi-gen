#ifndef MILA_AST_H
#define MILA_AST_H

#include <iostream>
#include <cassert>
#include "scanner.h"

namespace mila {
namespace ast {

class Visitor;

class Node {
public:
    int const line;
    int const col;

    virtual ~Node() {}

    virtual void accept(Visitor * v);

protected:
    Node(Token const & t):
        line(t.line),
        col(t.col) {
    }

};

class Expression;
class Variable;
class Number;
class Block;

class Declaration : public Node {
public:
    Symbol const symbol;
    Number * const value;

    Declaration(Token const & t, Number * value = nullptr):
        Node(t),
        symbol(t.symbol()),
        value(value) {
        assert (t == Token::Type::ident);
    }

    ~Declaration() override;

    void accept(Visitor * v) override;
};

class Arguments : public Node {
public:

    Arguments(Token const & t):
        Node(t) {
        assert (t == Token::Type::parOpen);
    }

    void accept(Visitor * v) override;
};


class Function : public Node {
public:

    Symbol const name;

    std::vector<Symbol> arguments;

    Node * const body;

    Function(Token t, std::vector<Symbol> && arguments, Node * body):
        Node(t),
        name(t.symbol()),
        arguments(arguments),
        body(body) {
        assert (t == Token::Type::ident);
    }

    ~Function() override;

    void accept(Visitor * v) override;
};

class Functions : public Node {
public:
    std::vector<Function *> functions;

    Functions(Token const & t):
        Node(t) {
    }

    ~Functions() override;

    void accept(Visitor * v) override;
};

class Declarations : public Node {
public:
    std::vector<Declaration *> declarations;

    Declarations(Token const & t):
        Node(t) {
    }

    ~Declarations() override;

    void accept(Visitor * v) override;
};

class Module : public Node {
public:
    Functions * const functions;
    Declarations * const declarations;
    Block * const body;

    Module(Token const & t, Functions * functions, Declarations * declarations, Block * body):
        Node(t),
        functions(functions),
        declarations(declarations),
        body(body) {
        assert (t == Token::Type::kwBegin);
    }

    ~Module() override;

    void accept(Visitor * v) override;
};

class Block : public Node {
public:
    Declarations * declarations;
    std::vector<Node *> statements;

    Block(Token const & t, Declarations * declarations):
        Node(t),
        declarations(declarations) {
        assert (t == Token::Type::kwBegin);
    }

    ~Block() override;

    void accept(Visitor * v) override;
};

class Write : public Node {
public:
    Expression * const expression;
    Write(Token const & t, Expression * expression):
        Node(t),
        expression(expression) {
        assert (t == Token::Type::kwWrite);
    }

    ~Write() override;

    void accept(Visitor * v) override;
};

class Read : public Node {
public:
    Symbol const symbol;
    Read(Token const & t, Symbol symbol):
        Node(t),
        symbol(symbol) {
        assert (t == Token::Type::kwRead);
    }

    void accept(Visitor * v) override;
};

class If : public Node {
public:
    Expression * const condition;
    Node * const trueCase;
    Node * const falseCase;

    If(Token const & t, Expression * condition, Node * trueCase, Node * falseCase = nullptr):
        Node(t),
        condition(condition),
        trueCase(trueCase),
        falseCase(falseCase) {
        assert (t == Token::Type::kwIf);
    }

    ~If() override;

    void accept(Visitor * v) override;
};

class While : public Node {
public:
    Expression * const condition;
    Node * const body;

    While(Token const & t, Expression * condition, Node * body):
        Node(t),
        condition(condition),
        body(body) {
        assert (t == Token::Type::kwWhile);
    }

    ~While() override;

    void accept(Visitor * v) override;
};

class Return : public Node {
public:
    Expression * const value;

    Return(Token const & t, Expression * value = nullptr):
        Node(t),
        value(value) {
        assert (t == Token::Type::kwReturn);
    }

    ~Return() override;

    void accept(Visitor * v) override;
};

class Expression : public Node {
public:
    void accept(Visitor * v) override;
protected:
    Expression(Token const & t):
        Node(t) {
    }
};

class Assignment : public Node {
public:
    Symbol const symbol;
    Expression * const value;

    Assignment(Token const & t, Expression * value):
        Node(t),
        symbol(t.symbol()),
        value(value) {
        assert (t == Token::Type::ident);
    }

    ~Assignment() override;

    void accept(Visitor * v) override;
};

class Call : public Expression {
public:
    Symbol const function;

    std::vector<Expression *> arguments;

    Call(Token const & t):
        Expression(t),
        function(t.symbol()) {
        assert (t == Token::Type::ident);
    }

    ~Call() override;

    void accept(Visitor * v) override;
};

class Binary : public Expression {
public:
    Token::Type const type;
    Expression * const lhs;
    Expression * const rhs;

    Binary(Token const & t, Expression * lhs, Expression * rhs):
        Expression(t),
        type(t.type),
        lhs(lhs),
        rhs(rhs) {
        assert (t == Token::Type::opAdd or
                t == Token::Type::opSub or
                t == Token::Type::opMul or
                t == Token::Type::opDiv or
                t == Token::Type::opEq or
                t == Token::Type::opNeq or
                t == Token::Type::opLt or
                t == Token::Type::opGt or
                t == Token::Type::opLte or
                t == Token::Type::opGte);
    }

    ~Binary() override;

    void accept(Visitor * v) override;
};

class Unary : public Expression {
public:
    Token::Type type;
    Expression * const operand;

    Unary(Token const & t, Expression * op):
        Expression(t),
        type(t.type),
        operand(op) {
        assert (t == Token::Type::opAdd or t == Token::Type::opSub);
    }

    ~Unary() override;

    void accept(Visitor * v) override;
};

class Variable : public Expression {
public:
    Symbol const symbol;
    Variable(Token const & from):
        Expression(from),
        symbol(from.symbol()) {
        assert(from == Token::Type::ident);
    }

    void accept(Visitor * v) override;
};

class Number : public Expression {
public:
    int const value;
    Number(Token const & from):
        Expression(from),
        value(from.value()) {
        assert(from == Token::Type::number);
    }

    void accept(Visitor * v) override;
};

class Visitor {
protected:

    friend class Node;
    friend class Declaration;
    friend class Arguments;
    friend class Function;
    friend class Functions;
    friend class Declarations;
    friend class Module;
    friend class Block;
    friend class Write;
    friend class Read;
    friend class If;
    friend class While;
    friend class Return;
    friend class Assignment;
    friend class Expression;
    friend class Call;
    friend class Binary;
    friend class Unary;
    friend class Variable;
    friend class Number;

    virtual ~Visitor() { }

    virtual void visit(Node * n) = 0;
    virtual void visit(Declaration * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Function * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Functions * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Declarations * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Module * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Block * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Write * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Read * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(If * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(While * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Return * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Assignment * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Expression * d) {
        return visit(static_cast<Node*>(d));
    }
    virtual void visit(Call * d) {
        return visit(static_cast<Expression*>(d));
    }
    virtual void visit(Binary * d) {
        return visit(static_cast<Expression*>(d));
    }
    virtual void visit(Unary * d) {
        return visit(static_cast<Expression*>(d));
    }
    virtual void visit(Variable * d) {
        return visit(static_cast<Expression*>(d));
    }
    virtual void visit(Number * d) {
        return visit(static_cast<Expression*>(d));
    }


};





}
}

#endif
