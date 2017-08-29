#include "server.hpp"
#include "config.hpp"
#include <SimpleJSON/utility/base64.hpp>

#include <tiny-process-library/process.hpp>
#include <boost/filesystem.hpp>
#include <attender/attender/session/session.hpp>
#include <attender/attender/mounting.hpp>
#include <attender/attender/session/memory_session_storage.hpp>
#include <attender/attender/session/uuid_session_cookie_generator.hpp>

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
        , sessions_{std::make_unique <memory_session_storage <uuid_generator, session>>()}
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
    void Server::loadConfig()
    {
        std::ifstream reader("./config.json", std::ios_base::binary);
        if (reader.good())
        {
            auto config = ::loadConfig(reader);

            std::stringstream temp;
            JSON::encodeBase64 <char> (temp, config.user + ":" + config.password);
            auth64_ = temp.str();

            for (auto const& i : config.projects)
            {
                if (i.type == (int)BuildType::batch)
                    addProject(i.id, i.rootDir, BatchBuild{});
                else
                    addProject(i.id, i.rootDir, BashBuild{});
            }
        }
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

        server_.get("/authenticate", [this](auto req, auto res)
        {
            attender::session sess;
            auto state = sessions_.load_session ("SESS", sess, req);
            if (state == session_state::live)
                sessions_.terminate_session(sess);

            auto auth = req->get_header_field("Authorization");
            
            if (!auth || auth.get() != "Basic " + auth64_)
            {
                res->send_status(403);
                return;
            }

            auto newSession = sessions_.make_session <attender::session> ();
            cookie ck;
            ck.set_name("SESS").set_value(newSession.id());
            res->set_cookie(ck);
            res->send_status(204);
        });
    }
//---------------------------------------------------------------------------------------------------------------------
#define AQUIRE_LOCK(ID) \
    std::unique_lock <std::mutex> guard{logAccess_}; \
    auto& entry = compileStatus_[ID]; \
    guard.unlock(); \
    std::lock_guard <std::mutex> guard2{entry.access}

#define REQUIRE_AUTH() \
    attender::session sess; \
    auto state = sessions_.load_session ("SESS", sess, req); \
    if (state != session_state::live) \
    { \
        res->send_status(401); \
        return; \
    }
//---------------------------------------------------------------------------------------------------------------------
    void Server::clearBuildLog(std::string const& id)
    {
        AQUIRE_LOCK(id);
        entry.log.clear();
    }
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
    int Server::getExitStatus(std::string const& id)
    {
        AQUIRE_LOCK(id);
        return entry.exitStatus;
    }
//---------------------------------------------------------------------------------------------------------------------
    void Server::addProject(std::string const& id, std::string const& rootDir, BuildConfig const& config)
    {
        // get build log
        server_.get("/"s + id + "_log", [this, id](auto req, auto res)
        {
            REQUIRE_AUTH()

            res->type(".txt").send(getBuildLog(id));
        });

        server_.get("/"s + id + "_running", [this, id](auto req, auto res)
        {
            REQUIRE_AUTH()

            res->type(".txt").send(std::to_string(isBuildRunning(id)));
        });

        server_.get("/"s + id + "_exit_status", [this, id](auto req, auto res)
        {
            REQUIRE_AUTH()

            res->type(".txt").send(std::to_string(getExitStatus(id)));
        });

        // mkdir
        server_.post("/"s + id + "_mkdir", [this, id, rootDir](auto req, auto res)
        {
            REQUIRE_AUTH()

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
                    clearBuildLog(id); \
                    Process builder(COMMAND, rootDir, \
                    [this, id](const char *bytes, size_t n) /* STDOUT */\
                    { \
                        appendBuildLog(id, {bytes, n}); \
                        std::cout << std::string{bytes, n}; \
                    }, \
                    [this, id](const char *bytes, size_t n) /* STDERR */\
                    { \
                        appendBuildLog(id, {bytes, n}); \
                        std::cout << std::string{bytes, n}; \
                    } \
                    , true); \
                    auto exitStatus = builder.get_exit_status(); \
                    AQUIRE_LOCK(id); \
                    entry.running = false; \
                    entry.exitStatus = exitStatus; \
                }); \
                buildThread->detach(); \
                res->send_status(204)

        // bash build
        if (config.type == BuildType::bash)
        {
            // issue build
            server_.get("/"s + id + "_build", [this, id, rootDir, config = *static_cast <BashBuild const*> (&config)](auto req, auto res)
            {
                REQUIRE_AUTH()

                BUILD_BY_SCRIPT("bash "s + rootDir + "/" + config.buildScript + " \"" + rootDir + "\"");
            });
        }
        else if (config.type == BuildType::batch)
        {
            // issue build
            server_.get("/"s + id + "_build", [this, id, rootDir, config = *static_cast <BatchBuild const*> (&config)](auto req, auto res)
            {
                REQUIRE_AUTH()

                BUILD_BY_SCRIPT(rootDir + "/" + config.buildScript + " \"" + rootDir + "\"");
            });
        }
        else
            throw std::invalid_argument("unknown build type");

        // mount
        server_.mount(rootDir, "/"s + id, [this, localOnly = config.localOnly](auto req, auto res)
        {
            attender::session sess;
            auto state = sessions_.load_session ("SESS", sess, req);
            if (state != session_state::live)
                return false;

            if (localOnly && req->ip().substr(0, 7) != "192.168")
                return false;
            return true;
        }, {mount_options::GET, mount_options::HEAD, mount_options::OPTIONS, mount_options::PUT, mount_options::POST, mount_options::DELETE});
    }
//#####################################################################################################################
}
