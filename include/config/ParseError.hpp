#ifndef PARSE_ERROR_HPP
#define PARSE_ERROR_HPP

#include <stdexcept>
#include <string>
#include <sstream>

class ParseError : public std::runtime_error
{
        private:
            size_t line_;

            static std::string compose(const std::string &msg, size_t line)
            {
                std::string s = "Config parse error at line ";
                std::ostringstream oss;
                oss << line;
                s += oss.str();
                s += ": ";
                s += msg;
                return s;
            }
    public:
        ParseError(const std::string &msg, size_t line): std::runtime_error(compose(msg, line)), line_(line) {}
        size_t line() const
        {
            return line_;
        }
};

#endif
