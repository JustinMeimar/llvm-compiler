#pragma once
#include <iostream>
#include <sstream>
// #include "AST.h"

namespace gazprea {

// Binary Operator Type Error
class BinaryOpTypeError : public std::exception {
    private:
        std::string msg;
    public: 
        BinaryOpTypeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos);
        virtual const char* what() const throw() {
            return msg.c_str();
        }
};

// Tuple Incompatible Size Error
class TupleSizeError : public std::exception {
    private:
        std::string msg;
    public: 
        TupleSizeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos);
        virtual const char* what() const throw() {
            return msg.c_str();
        }
};


} // namespace gazprea


