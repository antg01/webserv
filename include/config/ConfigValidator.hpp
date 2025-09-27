#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include "Config.hpp"
#include "ValidationError.hpp"
#include <string>

class ConfigValidator
{
    private:
        static bool isValidPort(int p);
        static bool pathExists(const std::string &p);
        static bool isDir(const std::string &p);
        static bool isFile(const std::string &p);
        static std::string joinPath(const std::string &a, const std::string &b);
        static std::string stripLeadingSlash(const std::string &p);
        static std::string normalizeRoot(const std::string &base, const std::string &root);
        static void validateServer(const ServerBlock &srv, size_t serverIndex, const std::string &baseDir);
        static void validateErrorPages(const ServerBlock &srv, const std::string &effectiveRoot, size_t serverIndex);
        static void validateLocations(const ServerBlock &srv, const std::string &effectiveRoot, size_t serverIndex);

    public:
        // Validate ports and filesystem paths.
        // baseDir is optional; if non-empty, it is used as a prefix for relative roots.
        static void validate(const Config &cfg, const std::string &baseDir = "");
};

#endif
