#ifndef LOCATION_BLOCK_HPP
#define LOCATION_BLOCK_HPP

#include <string>
#include <vector>
#include <set>
#include <map>

class LocationBlock
{
    private:
        std::string path_prefix;
        std::string root;
        std::set<std::string> methods;
        bool autoindex;
        std::vector<std::string> index_files;
        std::string upload_store;
        std::map<std::string, std::string> cgi_map;
        bool has_redirect;
        std::string redirect_code;
        std::string redirect_to;

    public:
        LocationBlock(): path_prefix(), root(), methods(), autoindex(false), index_files(), upload_store(), cgi_map(), has_redirect(false), redirect_code(), redirect_to() {}

        // Setters
        void setPathPrefix(const std::string &p)
        {
            path_prefix = p;
        }

        void setRoot(const std::string &p)
        {
            root = p;
        }

        void setAutoIndex(bool v)
        {
            autoindex = v;
        }

        void setUploadStore(const std::string &p)
        {
            upload_store = p;
        }

        void addIndex(const std::string &f)
        {
            index_files.push_back(f);
        }

        void addMethod(const std::string &m)
        {
            methods.insert(m);
        }

        void setRedirect(const std::string &code, const std::string &to)
        {
            has_redirect = true;
            redirect_code = code;
            redirect_to = to;
        }
        void mapCgi(const std::string &ext, const std::string &execPath)
        {
            cgi_map[ext] = execPath;
        }

        // Getters
        const std::string &getPathPrefix() const
        {
            return path_prefix;
        }

        const std::string &getRoot() const
        {
            return root;
        }

        const std::set<std::string> &getMethods() const
        {
            return methods;
        }

        bool getAutoIndex() const
        {
            return autoindex;
        }

        const std::vector<std::string> &getIndexFiles() const
        {
            return index_files;
        }

        const std::string &getUploadStore() const
        {
            return upload_store;
        }

        const std::map<std::string,std::string> &getCgiMap() const
        {
            return cgi_map;
        }

        bool hasRedirect() const
        {
            return has_redirect;
        }

        const std::string &getRedirectCode() const
        {
            return redirect_code;
        }

        const std::string &getRedirectTo() const
        {
            return redirect_to;
        }
};

#endif
