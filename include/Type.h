#pragma once

#include <string>

namespace gazprea {
    class Type {
    public:
        virtual std::string getName() = 0;
        virtual ~Type();
        virtual bool isTypedefType() = 0;
        virtual bool isMatrixType() = 0;
        virtual bool isIntervalType() = 0;
        virtual bool isTupleType() = 0;
    };
}