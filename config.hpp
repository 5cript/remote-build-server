#pragma once

#ifndef Q_MOC_RUN // A Qt workaround, for those of you who use Qt
#   include <SimpleJSON/parse/jsd_fusion_adapted_struct.hpp>
#   include <SimpleJSON/stringify/jss_fusion_adapted_struct.hpp>
#endif

#include <string>
#include <vector>

struct ProjectConfig : public JSON::Stringifiable <ProjectConfig>
                     , public JSON::Parsable <ProjectConfig>
{
    std::string id;
    std::string rootDir;
    int type;
};

struct Config : public JSON::Stringifiable <Config>
              , public JSON::Parsable <Config>
{
    std::vector <ProjectConfig> projects;
    std::string user;
    std::string password;
    boost::optional <int> port;
};

Config loadConfig(std::istream& json);
void saveConfig(std::ostream& stream, Config const& cfg);

BOOST_FUSION_ADAPT_STRUCT
(
    ProjectConfig,
    rootDir, id, type
)

BOOST_FUSION_ADAPT_STRUCT
(
    Config,
    projects, user, password, port
)
