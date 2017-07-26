#include "config.hpp"

#include <SimpleJSON/parse/jsd.hpp>
#include <SimpleJSON/stringify/jss.hpp>

//#####################################################################################################################
Config loadConfig(std::istream& json)
{
    Config cc;
    auto tree = JSON::parse_json(json);
    JSON::parse(cc, "config", tree);
    return cc;
}
//---------------------------------------------------------------------------------------------------------------------
void saveConfig(std::ostream& stream, Config const& cfg)
{
    stream << "{";
    JSON::try_stringify(stream, "config", cfg, JSON::ProduceNamedOutput);
    stream << "}";
}
//#####################################################################################################################
