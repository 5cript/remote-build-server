#pragma once

#include "build_type.hpp"

#include <string>

namespace RemoteBuild
{
    struct BuildConfig
    {
        std::string command = "bash";
        BuildType type = BuildType::bash;
        bool localOnly = false;
    };

    struct BashBuild : BuildConfig
    {
        BashBuild(std::string command = "bash",
                  std::string buildScript = "build.sh",
                  std::string cleanScript = "clean.sh")
            : BuildConfig{command, BuildType::bash, false}
            , buildScript{buildScript}
            , cleanScript{cleanScript}
        {}
        std::string buildScript;
        std::string cleanScript;
    };

    struct BatchBuild : BuildConfig
    {
        BatchBuild(std::string command = "",
                   std::string buildScript = "build.bat",
                   std::string cleanScript = "clean.bat")
            : BuildConfig{command, BuildType::batch, false}
            , buildScript{buildScript}
            , cleanScript{cleanScript}
        {}
        std::string buildScript;
        std::string cleanScript;
    };

}
