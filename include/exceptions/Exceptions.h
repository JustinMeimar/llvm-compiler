#pragma once
#include <iostream>
#include <sstream>
#include "BaseErrorListener.h"
#include "Recognizer.h"
#include "Token.h"
#include "Parser.h"

namespace gazprea {

// ANTLR Syntax Errors
class SyntaxError : public std::exception {
private:
    std::string msg;
public:
    SyntaxError(std::string msg) : msg(msg) {}
    virtual const char* what() const throw() {
        return msg.c_str();
    }
};

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

// Parallel Assigmnent Error
class ParallelAssignmentError : public std::exception {
    private:
        std::string msg;
    public: 
        ParallelAssignmentError(std::string rhs, std::string fullText, int line, int charPos);
        virtual const char* what() const throw() {
            return msg.c_str();
        }
};

// Unknown Id Error
class UndefinedIdError : public std::exception {
    private:
        std::string msg;
    public: 
        UndefinedIdError(std::string id, std::string fullText, int line, int charPos);
        virtual const char* what() const throw() {
            return msg.c_str();
        }
};

// Id Redefinition Error
class RedefineIdError : public std::exception {
    private:
        std::string msg;
    public: 
        RedefineIdError(std::string id, std::string fullText, int line, int charPos);
        virtual const char* what() const throw() {
            return msg.c_str();
        }
};

} // namespace gazprea


