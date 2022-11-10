#pragma once

#include <string>

namespace gazprea {
    class Type {
    public:
        enum {
            TUPLE = 0,
            INTERVAL = 1,
            BOOLEAN = 2,
            CHARACTER = 3,
            INTEGER = 4,
            REAL = 5,
            STRING = 6,
            INTEGER_INTERVAL = 7,
            BOOLEAN_1 = 8,
            CHARACTER_1 = 9,
            INTEGER_1 = 10,
            REAL_1 = 11,
            BOOLEAN_2 = 12,
            CHARACTER_2 = 13,
            INTEGER_2 = 14,
            REAL_2 = 15,
            IDENTITYNULL = 16,
        };
        virtual ~Type(); 
        virtual std::string getName() = 0;
        virtual bool isTypedefType() = 0;
        virtual bool isMatrixType() = 0;
        virtual bool isIntervalType() = 0;
        virtual bool isTupleType() = 0;
        virtual int getTypeId() = 0;
    };
}