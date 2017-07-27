#include "server.hpp"

#include <fstream>
#include <iostream>

int main()
{
    using namespace RemoteBuild;

    {
        std::ifstream reader("./config.json", std::ios_base::binary);
        if (!reader.good())
        {
            std::cerr << "provide config.json\n";
            return 1;
        }
    }

    Server server;
    server.loadConfig();
    server.start("26000");
    std::cin.get();
}
