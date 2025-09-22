#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <map>
#include <cctype>
#include <sstream>
#include <fstream>
#include "Config.hpp"
#include "ParseError.hpp"

struct Token
{
    enum Type
    {
        T_IDENT,       // word: server, location, root, etc.
        T_NUMBER,      // digits
        T_STRING,      // quoted "string"
        T_LBRACE,      // {
        T_RBRACE,      // }
        T_SEMI,        // ;
        T_EOF
    };

    Type        type;
    std::string text;
    size_t      line;

    Token(): type(T_EOF), text(), line(0) {}
    Token(Type t, const std::string &s, size_t ln) : type(t), text(s), line(ln) {}
};

class Lexer
{
    private:
        std::string src_;
        size_t      pos_;
        size_t      line_;

        void skipSpacesAndComments();
        bool eof() const;
        char peek() const;
        char get();
        Token readIdent();
        Token readNumberOrIdent();
        Token readString();

    public:
        Lexer() : line_(1) {}

        void load(const std::string &path);
        Token next();

};

class ConfigParser
{
    private:
        Lexer   lex_;
        Token   cur_;

        void    advance();
        bool    accept(Token::Type t);
        void    expect(Token::Type t, const char *what);

        Config          parseConfig();
        ServerBlock     parseServer();
        LocationBlock   parseLocation(ServerBlock &server, const std::string &prefix);

        // server-level directives
        void parseServerDirective(ServerBlock &srv);
        void dir_listen(ServerBlock &srv);
        void dir_root(ServerBlock &srv);
        void dir_index(ServerBlock &srv);
        void dir_server_name(ServerBlock &srv);
        void dir_client_max_body_size(ServerBlock &srv);
        void dir_error_page(ServerBlock &srv);
        void dir_location(ServerBlock &srv);

        // location-level directives
        void parseLocationDirective(LocationBlock &loc, const ServerBlock &parent);
        void loc_root(LocationBlock &loc);
        void loc_methods(LocationBlock &loc);
        void loc_autoindex(LocationBlock &loc);
        void loc_index(LocationBlock &loc);
        void loc_upload_store(LocationBlock &loc);
        void loc_return(LocationBlock &loc);
        void loc_cgi_pass(LocationBlock &loc);

        // helpers
        static size_t parseSizeWithUnit(const std::string &s); // e.g. 10M, 512K
        static void splitHostPort(const std::string &s, std::string &host, int &port);

    public:
        ConfigParser();
        Config parseFile(const std::string &path);
};

#endif
