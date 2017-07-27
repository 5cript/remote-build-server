#pragma once

#include "build_config.hpp"

#include <attender/attender.hpp>
#include <attender/attender/session/session_manager.hpp>

#include <unordered_map>
#include <utility>
#include <mutex>

namespace RemoteBuild
{
    struct BuildStatus
    {
        std::mutex access;
        std::string log;
        bool running;
        int exitStatus;
    };

    class Server
    {
    public:
        Server();

        /**
         *  Load projects from config.
         */
        void loadConfig();

        /**
         *  Mount project directory.
         *
         *  @param id The url unique identifier for the project.
         *  @param rootDir The project directory (location of the build script / system).
         *  @param config Build type etc.
         */
        void addProject(std::string const& id, std::string const& rootDir, BuildConfig const& config);

        /**
         *  Starts the server
         */
        void start(std::string const& port);

    private:
        void initializeRoutings();

        void clearBuildLog(std::string const& id);
        void setBuildRunning(std::string const& id, bool running);
        bool isBuildRunning(std::string const& id);
        void appendBuildLog(std::string const& id, std::string const& text);
        std::string getBuildLog(std::string const& id);
        int getExitStatus(std::string const& id);

    private:
        attender::managed_io_context <attender::thread_pooler> ctx_;
        attender::session_manager sessions_;
        attender::tcp_server server_;

        std::mutex logAccess_;
        std::unordered_map <std::string, BuildStatus> compileStatus_;
    };
}
