// MAIN DE TEST POUR PARSER CONFIG

// src/main.cpp
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "config/ConfigParser.hpp"
#include "config/Config.hpp"
#include "config/ServerBlock.hpp"
#include "config/LocationBlock.hpp"
#include "config/ParseError.hpp"
#include "utils/PathUtils.hpp"
#include "config/ConfigValidator.hpp"


static void printServer(const ServerBlock& srv, size_t idx)
{
    std::cout << "=== Server #" << idx << " ===\n";
    // listens
    const std::vector<ListenEntry>& ls = srv.getListens();
    std::cout << "listens: ";
    for (std::vector<ListenEntry>::const_iterator it = ls.begin(); it != ls.end(); ++it)
    {
        if (it != ls.begin()) std::cout << ", ";
        std::cout << it->host << ":" << it->port;
    }
    std::cout << "\n";

    // root, server_name, max body
    std::cout << "root: " << srv.getRoot() << "\n";
    std::cout << "server_name: " << srv.getServerName() << "\n";
    std::cout << "client_max_body_size: " << srv.getClientMaxBodySize() << "\n";

    // index files
    const std::vector<std::string>& idxs = srv.getIndexFiles();
    std::cout << "index: ";
    for (std::vector<std::string>::const_iterator it = idxs.begin(); it != idxs.end(); ++it)
    {
        if (it != idxs.begin()) std::cout << " ";
        std::cout << *it;
    }
    std::cout << "\n";

    // error pages
    const std::map<int,std::string>& eps = srv.getErrorPages();
    std::cout << "error_pages:\n";
    for (std::map<int,std::string>::const_iterator it = eps.begin(); it != eps.end(); ++it)
    {
        std::cout << "  " << it->first << " -> " << it->second << "\n";
    }

    // locations
    const std::vector<LocationBlock>& locs = srv.getLocations();
    size_t i = 0;
    for (; i < locs.size(); ++i)
    {
        const LocationBlock& loc = locs[i];
        std::cout << "  - location: " << loc.getPathPrefix() << "\n";
        std::cout << "    root: " << loc.getRoot() << "\n";
        std::cout << "    autoindex: " << (loc.getAutoIndex() ? "on" : "off") << "\n";

        // methods
        std::cout << "    methods: ";
        const std::set<std::string>& ms = loc.getMethods();
        for (std::set<std::string>::const_iterator mit = ms.begin(); mit != ms.end(); ++mit)
        {
            if (mit != ms.begin()) std::cout << " ";
            std::cout << *mit;
        }
        std::cout << "\n";

        // index files
        std::cout << "    index: ";
        const std::vector<std::string>& lidx = loc.getIndexFiles();
        for (std::vector<std::string>::const_iterator it2 = lidx.begin(); it2 != lidx.end(); ++it2)
        {
            if (it2 != lidx.begin()) std::cout << " ";
            std::cout << *it2;
        }
        std::cout << "\n";

        // upload store
        std::cout << "    upload_store: " << loc.getUploadStore() << "\n";

        // redirect
        std::cout << "    redirect: ";
        if (loc.hasRedirect())
        {
            std::cout << loc.getRedirectCode() << " " << loc.getRedirectTo() << "\n";
        }
        else
        {
            std::cout << "(none)\n";
        }

        // cgi map
        std::cout << "    cgi_map:\n";
        const std::map<std::string,std::string>& cg = loc.getCgiMap();
        for (std::map<std::string,std::string>::const_iterator cit = cg.begin(); cit != cg.end(); ++cit)
        {
            std::cout << "      " << cit->first << " -> " << cit->second << "\n";
        }
    }
    std::cout << std::endl;
}

int main(int argc, char** argv)
{
    std::cout << "===== resolvePath test========" << std::endl;

    std::string root = "./www/site1";
    std::string req1 = "/static/../index.html";
    std::string req2 = "/../../etc/passwd";
    std::string req3 = "/images/pic.png";

    std::cout << resolvePath(root, req1) << std::endl;
    std::cout << resolvePath(root, req2) << std::endl;
    std::cout << resolvePath(root, req3) << std::endl;

    std::cout << "===== parser && port validity test========" << std::endl;

    // Usage: ./webserv <config_file>
    std::string path = "conf/default.conf";
    if (argc > 1) path = argv[1];

    try
    {
        ConfigParser parser;
        Config cfg = parser.parseFile(path);

        // test ports validity !!
        // ConfigValidator::validate(cfg, "");

        const std::vector<ServerBlock>& servers = cfg.getServers();
        if (servers.empty())
        {
            std::cout << "[info] No servers defined in config.\n";
            return 0;
        }

        std::cout << "[info] Loaded config with " << servers.size() << " server(s)\n";

        for (size_t i = 0; i < servers.size(); ++i)
        {
            printServer(servers[i], i);
        }

        // Example: collect all listens for the I/O team bootstrap
        std::vector<ListenEntry> listens = cfg.collectAllListens();
        std::cout << "[info] Collected " << listens.size() << " listen entries\n";
        return 0;
    }
    catch (const ParseError& e)
    {
        std::cerr << "[parse-error] " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[error] " << e.what() << std::endl;
        return 1;
    }
}

