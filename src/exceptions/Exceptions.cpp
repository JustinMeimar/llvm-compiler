#include "Exceptions.h"

namespace gazprea {

//Binary op between incompatible Types Error
BinaryOpTypeError::BinaryOpTypeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "cannot perform binary operation between "
            << "\033[36m" << lhs  << "\033[0m" << 
            " and " 
            << "\033[36m" << rhs  << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t  | \n" << "\t" << line << " |" << " '" << fullText << "'\n" << "\t  | \n";
    msg = sstream.str();
} 

//Tuples of Incompatible Size Error 
TupleSizeError::TupleSizeError(std::string lhs, std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "incompatible tuples sizes for "
            << "\033[36m" << lhs  << "\033[0m" << 
            " and " 
            << "\033[36m" << rhs  << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"

            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
} 

//Parallel Assignment Error
ParallelAssignmentError::ParallelAssignmentError(std::string rhs, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "invlaid value in parallel assignement "
            << "\033[36m" << rhs << "\033[0m" << 
            " is not of type: " 
            << "\033[36m" << "tuple" << "\033[0m" <<  
            " " << line << "\033[0m"<< ":" << charPos << "\n"

            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

UndefinedIdError::UndefinedIdError(std::string id, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "id '"
            << "\033[36m" << id << "\033[0m" << 
            "' can not be resolved in this scope" 
            " " << line << "\033[0m"<< ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

RedefineIdError::RedefineIdError(std::string id, std::string fullText, int line, int charPos) {
    std::stringstream sstream;
    sstream << "previously defined id"
            << "\033[36m" << id << "\033[0m" << 
            " can not be redefined " 
            " " << line << ":" << charPos << "\n"
            << "\t|\n"
            << line << "\t|" << " '" << fullText << "'\n"
            << "\t|\n";
    msg = sstream.str();
}

} // namespace gazprea

