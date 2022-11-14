#include "Exceptions.h"

namespace gazprea {

//Binary op between incompatible Types Error
BinaryOpTypeError::BinaryOpTypeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "error: Cannot perform binary operation between "
            << "\033[36m" << lhs  << "\033[0m" << 
            " and " 
            << "\033[36m" << rhs  << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"

            << "\t  | \n"
            << "\t" << line << " |" << " '" << fullText << "'\n"
            << "\t  | \n";

    msg = sstream.str();
} 

//Tuples of Incompatible Size Error 
TupleSizeError::TupleSizeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "error: incompatible tuples sizes "
            << "\033[36m" << lhs  << "\033[0m" << 
            " and " 
            << "\033[36m" << rhs  << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"

            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";

    msg = sstream.str();
} 



} // namespace gazprea

