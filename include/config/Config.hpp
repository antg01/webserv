#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include "ServerBlock.hpp"

class Config
{
    private:
        std::vector<ServerBlock> servers;

    public:
        void addServer(const ServerBlock &s)
        {
            servers.push_back(s);
        }

        const std::vector<ServerBlock> &getServers() const
        {
            return servers;
        }

        std::vector<ListenEntry> collectAllListens() const
        {
            std::vector<ListenEntry> out;
            for (std::vector<ServerBlock>::const_iterator it = servers.begin(); it != servers.end(); ++it)
            {
                const std::vector<ListenEntry>& v = it->getListens();
                for (std::vector<ListenEntry>::const_iterator jt = v.begin(); jt != v.end(); ++jt)
                {
                    out.push_back(*jt);
                }
            }
            return out;
        }
};

#endif
