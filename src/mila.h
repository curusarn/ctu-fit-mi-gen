#ifndef MILA_H
#define MILA_H

#include <ciso646>
#include <string>
#include <sstream>
#include <cassert>

#define UNREACHABLE assert(false)

#define STR(what) static_cast<std::stringstream&>(std::stringstream() << what).str()

namespace mila {

class Exception : public std::exception {
public:

    Exception(std::string const & message):
        message(message) {
    }

    char const * what() const noexcept override {
        return message.c_str();
    }

    std::string const message;
};


}


#endif
