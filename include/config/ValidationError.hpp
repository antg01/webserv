#ifndef VALIDATION_ERROR_HPP
#define VALIDATION_ERROR_HPP

#include <stdexcept>
#include <string>

class ValidationError : public std::runtime_error
{
    public:
        explicit ValidationError(const std::string &msg): std::runtime_error(msg) {}
};

#endif
