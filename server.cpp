#include "server.hpp"

#include <tiny-process-library/process.hpp>
#include <boost/filesystem.hpp>
#include <attender/attender/mounting.hpp>

#include <thread>
#include <iostream>

namespace RemoteBuild
{
    using namespace std::string_literals;
    using namespace attender;
    using namespace TinyProcessLib;
//#####################################################################################################################
    Server::Server()
        : ctx_{}
        , server_{ctx_.get_io_service(), [](auto* connection, auto const& ec, auto const& exc)
        {
            // some error occured. (this is not thread safe)
            // You MUST check the error code here, because some codes mean, that the connection went kaputt!
            // Accessing the connection might be invalid then / crash you.
            if (ec.value() == boost::system::errc::protocol_error)
            {
                std::cout << connection->get_remote_address() << ":" << connection->get_remote_port() << "\n";
            }
            std::cerr << ec << " " << exc.what() << "\n";
        }}
    {

    }
//---------------------------------------------------------------------------------------------------------------------
    void Server::start(std::string const& port)
    {
        initializeRoutings();
        server_.start(port);
    }
//---------------------------------------------------------------------------------------------------------------------
    void Server::initializeRoutings()
    {
        server_.get("/", [](auto req, auto res)
        {
            if (!res->type(".html").send_file("index.html"))
            {
                res->send_status(404);
            }
        });
    }
//---------------------------------------------------------------------------------------------------------------------
#define AQUIRE_LOCK(ID) \
    std::unique_lock <std::mutex> guard{logAccess_}; \
    auto& entry = compileStatus_[ID]; \
    guard.unlock(); \
    std::lock_guard <std::mutex> guard2{entry.access}
//---------------------------------------------------------------------------------------------------------------------
    void Server::setBuildRunning(std::string const& id, bool running)
    {
        AQUIRE_LOCK(id);
        entry.running = running;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool Server::isBuildRunning(std::string const& id)
    {
        AQUIRE_LOCK(id);
        return entry.running;
    }
//---------------------------------------------------------------------------------------------------------------------
    void Server::appendBuildLog(std::string const& id, std::string const& text)
    {
        AQUIRE_LOCK(id);
        entry.log += text;
    }
//---------------------------------------------------------------------------------------------------------------------
    std::string Server::getBuildLog(std::string const& id)
    {
        AQUIRE_LOCK(id);
        return entry.log;
    }
//---------------------------------------------------------------------------------------------------------------------
    void Server::addProject(std::string const& id, std::string const& rootDir, BuildConfig const& config)
    {
        // get build log
        server_.get("/"s + id + "_log", [this, id](auto req, auto res)
        {
            res->type(".txt").send(getBuildLog(id));
        });

        server_.get("/"s + id + "_running", [this, id](auto req, auto res)
        {
            res->type(".txt").send(std::to_string(isBuildRunning(id)));
        });

        // mkdir
        server_.post("/"s + id + "_mkdir", [this, id, rootDir](auto req, auto res)
        {
            namespace fs = boost::filesystem;

            auto dir = std::make_shared <std::string>();
            req->read_body(*dir).then(
                [dir{dir}, res, rootDir]()
                {
                    if (!attender::validate_path(*dir))
                        res->send_status(403);
                    else
                    {
                        std::replace(dir->begin(), dir->end(), '\\', '/');
                        auto path = fs::path(rootDir) / fs::path(*dir);
                        if (fs::exists(path))
                        {
                            res->send_status(204);
                            return;
                        }

                        auto createResult = boost::filesystem::create_directories(path);
                        if (createResult)
                            res->send_status(204);
                        else
                            res->send_status(400);
                    }
                }
            ).except(
                [](boost::system::error_code ec)
                {
                    // ignore
                }
            );
        });

        #define BUILD_BY_SCRIPT(COMMAND) \
            std::shared_ptr <std::thread> buildThread; \
                buildThread = std::make_shared <std::thread> ([this, id, buildThread, rootDir, config]() \
                { \
                    setBuildRunning(id, true); \
                    Process builder(COMMAND, rootDir, [this, id](const char *bytes, size_t n) \
                    { \
                        appendBuildLog(id, {bytes, n}); \
                        std::cout << std::string{bytes, n}; \
                    }, nullptr, true); \
                    builder.get_exit_status(); \
                    setBuildRunning(id, false); \
                }); \
                buildThread->detach(); \
                res->send_status(204)

        // bash build
        if (config.type == BuildType::bash)
        {
            // issue build
            server_.get("/"s + id + "_build", [this, id, rootDir, config = *static_cast <BashBuild const*> (&config)](auto req, auto res)
            {
                BUILD_BY_SCRIPT("bash "s + rootDir + "/" + config.buildScript + " \"" + rootDir + "\"");
            });
        }
        else if (config.type == BuildType::batch)
        {
            // issue build
            server_.get("/"s + id + "_build", [this, id, rootDir, config = *static_cast <BatchBuild const*> (&config)](auto req, auto res)
            {
                BUILD_BY_SCRIPT(rootDir + "/" + config.buildScript + " \"" + rootDir + "\"");
            });
        }
        else
            throw std::invalid_argument("unknown build type");

        // mount
        server_.mount(rootDir, "/"s + id, [localOnly = config.localOnly](auto req, auto res)
        {
            if (localOnly && req->ip().substr(0, 7) != "192.168")
                return false;
            return true;
        }, {mount_options::GET, mount_options::HEAD, mount_options::OPTIONS, mount_options::PUT, mount_options::POST, mount_options::DELETE});
    }
//#####################################################################################################################
}
