#include "scanner.h"

namespace mila {

std::map<std::string, int> Symbol::symbols;

std::map<std::string, Token::Type> Scanner::keywords({
    { "var", Token::Type::kwVar },
    { "const", Token::Type::kwConst },
    { "begin", Token::Type::kwBegin },
    { "end", Token::Type::kwEnd },
    { "if", Token::Type::kwIf },
    { "then", Token::Type::kwThen },
    { "else", Token::Type::kwElse },
    { "while", Token::Type::kwWhile },
    { "do", Token::Type::kwDo },
    { "write", Token::Type::kwWrite },
    { "read", Token::Type::kwRead },
    { "function", Token::Type::kwFunction },
    { "return", Token::Type::kwReturn },
});

}
