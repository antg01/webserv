#ifndef SERVER_BLOCK_HPP
#define SERVER_BLOCK_HPP

#include <string>
#include <vector>
#include <map>
#include "LocationBlock.hpp"

struct ListenEntry
{
    std::string host;
    int         port;

    ListenEntry(const std::string &h, int p) : host(h), port(p) {}
};

class ServerBlock
{
    private:
        std::vector<ListenEntry>   listens;
        std::string                root;
        std::vector<std::string>   index_files;
        std::string                server_name;
        size_t                     client_max_body_size;
        std::map<int,std::string>  error_pages;
        std::vector<LocationBlock> locations;

    public:
        ServerBlock(): listens(), root(), index_files(), server_name(), client_max_body_size(0), error_pages(), locations() {}

        void addListen(const std::string &host, int port)
        {
            listens.push_back(ListenEntry(host, port));
        }

        void setRoot(const std::string &p)
        {
            root = p;
        }

        void addIndex(const std::string &f)
        {
            index_files.push_back(f);
        }

        void setServerName(const std::string &n)
        {
            server_name = n;
        }

        void setClientMaxBodySize(size_t n)
        {
            client_max_body_size = n;
        }

        void setErrorPage(int code, const std::string &path)
        {
            error_pages[code] = path;
        }

        void addLocation(const LocationBlock &loc)
        {
            locations.push_back(loc);
        }

        const std::vector<ListenEntry> &getListens() const
        {
            return listens;
        }

        const std::string &getRoot() const
        {
            return root;
        }

        const std::vector<std::string> &getIndexFiles() const
        {
            return index_files;
        }

        const std::string &getServerName() const
        {
            return server_name;
        }

        size_t getClientMaxBodySize() const
        {
            return client_max_body_size;
        }

        const std::map<int,std::string> &getErrorPages() const
        {
            return error_pages;
        }

        const std::vector<LocationBlock> &getLocations() const
        {
            return locations;
        }
};

#endif
