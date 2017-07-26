#include "server.hpp"
#include "config.hpp"

#include <fstream>
#include <iostream>

int main()
{
    using namespace RemoteBuild;

    std::ifstream reader("./config.json", std::ios_base::binary);
    if (!reader.good())
    {
        std::cerr << "provide config.json\n";
        return 1;
    }
    auto config = loadConfig(reader);

    Server server;

    for (auto const& i : config.projects)
    {
        if (i.type == (int)BuildType::batch)
            server.addProject(i.id, i.rootDir, BatchBuild{});
        else
            server.addProject(i.id, i.rootDir, BashBuild{});
    }


    server.start("10101");
    std::cin.get();
}
