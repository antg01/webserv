#ifndef DIRECTORY_RESOLVER_HPP
#define DIRECTORY_RESOLVER_HPP

#include <string>
#include <vector>

enum DirDecision
{
    ServeIndexFile,
    ServeAutoIndex,
    Forbidden
};

struct DirResolveResult
{
    DirDecision  decision;
    std::string  file_path;
    std::string  html;
};

bool isDirectory(const std::string &absPath);
bool fileExists(const std::string &absPath);

// Look for first existing index from the provided list, return true and fill outPath if found.
bool findIndexFile(const std::string &dirAbsPath, const std::vector<std::string> &indexList, std::string &outPath);

// Main entry: decide what to serve for a directory request.
DirResolveResult resolveDirectoryRequest(const std::string &dirAbsPath, const std::string &urlPath, const std::vector<std::string> &indexList, bool autoindexOn);

#endif
