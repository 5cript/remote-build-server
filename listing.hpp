#pragma once

#include <string>
#include <vector>

#include <SimpleJSON/parse/jsd_fusion_adapted_struct.hpp>
#include <SimpleJSON/stringify/jss_fusion_adapted_struct.hpp>

namespace RemoteBuild
{
    struct FileWithHash : public JSON::Stringifiable <FileWithHash>
                        , public JSON::Parsable <FileWithHash>
    {
        std::string entry;
        std::string hash;

        FileWithHash(std::string entry = {}, std::string hash = {})
            : entry{std::move(entry)}
            , hash{std::move(hash)}
        {
        }
    };

    struct DirectoryListing : public JSON::Stringifiable <DirectoryListing>
                            , public JSON::Parsable <DirectoryListing>
    {
        std::string filter;
        std::string root;
        std::vector <FileWithHash> entriesWithHash;
    };

    /**
     *  @param root Path to recurse through
     *  @param extensionWhiteSelect Will only select files with the given extension. does not filter if empty.
     */
    DirectoryListing makeListing(std::string const& root, bool directories = false, std::string const& globber = {});
}

BOOST_FUSION_ADAPT_STRUCT
(
    RemoteBuild::FileWithHash,
    entry, hash
)

BOOST_FUSION_ADAPT_STRUCT
(
    RemoteBuild::DirectoryListing,
    root, filter, entriesWithHash
)
