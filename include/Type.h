#pragma once

#include <string>

namespace gazprea {
    class Type {
    public:
        virtual std::string getName() = 0;
        virtual ~Type();
    };
}