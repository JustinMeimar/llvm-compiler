#pragma once

#include <string>

namespace gazprea {
    class Type {
    public:
        enum {
            TUPLE = 0,
            BOOLEAN = 1,
            CHARACTER = 2,
            INTEGER = 3,
            REAL = 4,
            STRING = 5,
            INTEGER_INTERVAL = 6,
            BOOLEAN_1 = 7,
            CHARACTER_1 = 8,
            INTEGER_1 = 9,
            REAL_1 = 10,
            BOOLEAN_2 = 11,
            CHARACTER_2 = 12,
            INTEGER_2 = 13,
            REAL_2 = 14
        };
        virtual std::string getName() = 0;
        virtual ~Type();
        virtual bool isTypedefType() = 0;
        virtual bool isMatrixType() = 0;
        virtual bool isIntervalType() = 0;
        virtual bool isTupleType() = 0;
        virtual int getTypeId() = 0;
    };
}