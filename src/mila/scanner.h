#ifndef MILA_SCANNER_H
#define MILA_SCANNER_H

#include <cassert>

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>

#include "mila.h"

namespace mila {

class Symbol {
public:

    Symbol(std::string const & name) {
        auto i = symbols.find(name);
        if (i == symbols.end()) {
            id_ = symbols.size();
            symbols[name] = id_;
        } else {
            id_ = i->second;
        }
    }

    Symbol(Symbol const & other) = default;

    Symbol & operator = (Symbol const & other) = default;

    std::string const & name() const {
        static std::string unknown("unknown symbol");
        for (auto i = symbols.begin(), e = symbols.end(); i != e; ++i)
            if (i->second == id_)
                return i->first;
        return unknown;
    }

    bool operator == (char const * other) const {
        return name() == other;
    }

    bool operator != (char const * other) const {
        return name() != other;
    }

    bool operator == (Symbol const & other) const {
        return id_ == other.id_;
    }

    bool operator != (Symbol const & other) const {
        return id_ != other.id_;
    }

    operator char const * () const {
        return name().c_str();
    }


private:

    friend class Token;

    friend std::ostream & operator << (std::ostream & stream, Symbol const & symbol) {
        stream << symbol.name();
        return stream;
    }

    int id_;

    static std::map<std::string, int> symbols;

};

class Token {
public:
    enum class Type {
        ident, // identifier
        number, // integer number
        opAdd, // +
        opSub, // -
        opMul, // *
        opDiv, // /
        opLt, // <
        opGt, // >
        opLte, // <=
        opGte, // >=
        opEq, // =
        opNeq, // <>
        opAssign, // :=
        parOpen, // (
        parClose, // )
        comma, // ,
        colon, // :
        semicolon, // ;
        kwVar, // var
        kwConst, // const
        kwBegin, // begin
        kwEnd, // end
        kwIf, // if
        kwThen, // then
        kwElse, // else
        kwWhile, // while
        kwDo, // do
        kwWrite, // write
        kwRead, // read
        kwFunction, // function
        kwReturn, // return
        eof,
    };

    static char const * typeToString(Type t) {
        switch (t) {
        case Type::ident:
            return "identifier";
        case Type::number:
            return "number";
        case Type::opAdd:
            return "addition (+)";
        case Type::opSub:
            return "subtraction (-)";
        case Type::opMul:
            return "multiplication (*)";
        case Type::opDiv:
            return "division (/)";
        case Type::opLt:
            return "less (<)";
        case Type::opGt:
            return "greater (>)";
        case Type::opLte:
            return "less or equal (<=)";
        case Type::opGte:
            return "greater or equal (>=)";
        case Type::opEq:
            return "equals (=)";
        case Type::opNeq:
            return "not equals (<>)";
        case Type::opAssign:
            return "assignment (:=)";
        case Type::parOpen:
            return "opening parenthesis";
        case Type::parClose:
            return "closing parenthesis";
        case Type::comma:
            return "comma";
        case Type::colon:
            return "colon";
        case Type::semicolon:
            return "semicolon";
        case Type::kwVar:
            return "var keyword";
        case Type::kwConst:
            return "const keyword";
        case Type::kwBegin:
            return "begin keyword";
        case Type::kwEnd:
            return "end keyword";
        case Type::kwIf:
            return "if keyword";
        case Type::kwThen:
            return "then keyword";
        case Type::kwElse:
            return "else keyword";
        case Type::kwWhile:
            return "while keyword";
        case Type::kwDo:
            return "do keyword";
        case Type::kwWrite:
            return "write keyword";
        case Type::kwRead:
            return "read keyword";
        case Type::kwFunction:
            return "function keyword";
        case Type::kwReturn:
            return "return keyword";
        case Type::eof:
            return "end of file";
        default:
            UNREACHABLE;
        }
    }

    Type const type;

    int const line;
    int const col;

    int value() const {
        assert(type == Token::Type::number);
        return value_;
    }

    Symbol const & symbol() const {
        assert(type == Token::Type::ident);
        return symbol_;
    }

    static Token eof(int line, int col) {
        return Token(Type::eof, 0, line, col);
    }

    static Token create(Type type, int line, int col) {
        return Token(type, 0, line, col);
    }

    static Token number(int value, int line, int col) {
        return Token(Type::number, value, line, col);
    }

    static Token identifier(std::string const & value, int line, int col) {
        return Token(Type::ident, Symbol(value).id_, line, col);
    }

    bool operator == (Token::Type t) const {
        return type == t;
    }

    bool operator != (Token::Type t) const {
        return type != t;
    }


private:
    Token(Type type, int payload, int line, int col):
        type(type),
        line(line),
        col(col),
        value_(payload) {
    }

    union {
        int const value_;
        Symbol const symbol_;
    } ;


};

inline std::ostream & operator << (std::ostream & stream, Token const & t) {
    switch (t.type) {
    case Token::Type::ident:
        stream << "identifier " << t.symbol();
        break;
    case Token::Type::number:
        stream << "number " << t.value();
        break;
    default:
        stream << Token::typeToString(t.type);
    }
    stream << " (line " << t.line << ", col " << t.col << ")";
    return stream;
}

class ScannerError : public Exception {
public:
    ScannerError(std::string const & message, int line, int col):
        Exception(STR(message << " (line: " << line << ", col: " << col << ")")) {
    }
};

/** mila++ scanner.
 */
class Scanner {
public:
    static Scanner file(std::string const & filename) {
        std::ifstream s(filename);
        if (not s.is_open())
            throw Exception(STR("Unable to open file " << filename));
        return Scanner(s);
    }

    static Scanner text(std::string const & text) {
        std::stringstream ss(text);
        return Scanner(ss);
    }

    size_t size() {
        return tokens.size();
    }

    Token const & top() {
        return tokens[current];
    }

    Token const & pop() {
        Token const & result = top();
        if (result != Token::Type::eof)
            ++current;
        return result;
    }

    void revert() {
        --current;
    }

    bool eof() {
        return top() == Token::Type::eof;
    }

private:

    explicit Scanner(std::istream & input):
        line(1),
        col(1),
        current(0) {
        while (true) {
            Token t = next(input);
            tokens.push_back(t);
            if (t == Token::Type::eof)
                break;
        }
    }

    bool isLetter(char c) {
        return (c >= 'A' and c <= 'Z') or (c >= 'a' and c<= 'z');
    }

    bool isDigit(char c) {
        return c >= '0' and c <= '9';
    }

    bool isWhitespace(char c) {
        return c == ' ' or c == '\t' or c == '\n' or c == '\r';
    }

    char get(std::istream & input) {
        char result = input.get();
        if (result == '\n') {
            col = 1;
            line += 1;
        } else {
            col += 1;
        }
        return result;
    }

    bool condGet(std::istream & input, char what) {
        if (input.peek() == what) {
            get(input);
            return true;
        } else {
            return false;
        }
    }

    Token number(std::istream & input, char t, int l, int c) {
        int result = t - '0';
        while (not input.eof() and isDigit(input.peek()))
            result = result * 10 + (get(input) - '0');
        return Token::number(result, l, c);
    }

    Token identifierOrKeyword(std::istream & input, char t, int l, int c) {
        std::string result;
        while (true) {
            result += t;
            t = input.peek();
            if (input.eof() or not (isDigit(t) or isLetter(t)))
                break;
            get(input);
        }
        auto i = keywords.find(result);
        if (i == keywords.end())
            return Token::identifier(result, l, c);
        else
            return Token::create(i->second, l, c);
    }

    Token next(std::istream & input) {
        int l = line;
        int c = col;
        char t = get(input);
        // skip the whitespace and comments
        bool skip = false;
        while (skip or t == ' ' or t == '\n' or t == '\t' or t == '\r' or t == '{') {
            if (t == '{')
                skip = true;
            if (skip and t == '}')
                skip = false;
            l = line;
            c = col;
            t = get(input);
        }
        if (input.eof())
            return Token::eof(line, col);
        switch (t) {
        case '+':
            return Token::create(Token::Type::opAdd, l, c);
        case '-':
            return Token::create(Token::Type::opSub, l, c);
        case '*':
            return Token::create(Token::Type::opMul, l, c);
        case '/':
            return Token::create(Token::Type::opDiv, l, c);
        case '(':
            return Token::create(Token::Type::parOpen, l, c);
        case ')':
            return Token::create(Token::Type::parClose, l, c);
        case '=':
            return Token::create(Token::Type::opEq, l, c);
        case ',':
            return Token::create(Token::Type::comma, l, c);
        case ':':
            if (condGet(input, '='))
                return Token::create(Token::Type::opAssign, l, c);
            return Token::create(Token::Type::colon, l, c);
        case ';':
            return Token::create(Token::Type::semicolon, l, c);
        case '<':
            if (condGet(input, '>'))
                return Token::create(Token::Type::opNeq, l, c);
            if (condGet(input, '='))
                return Token::create(Token::Type::opLte, l, c);
            return Token::create(Token::Type::opLt, l, c);
        case '>':
            if (condGet(input, '='))
                return Token::create(Token::Type::opGte, l, c);
            return Token::create(Token::Type::opGt, l, c);
         default:
            if (isDigit(t))
                return number(input, t, l, c);
            if (isLetter(t))
                return identifierOrKeyword(input, t, l, c);
            throw ScannerError(STR("Unknown character " << t), l, c);
        }
    }

    int line;
    int col;

    size_t current;

    std::vector<Token> tokens;

    static std::map<std::string, Token::Type> keywords;
};





}



#endif
