#include "listing.hpp"

#include <boost/filesystem.hpp>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

namespace RemoteBuild
{
//#####################################################################################################################
    std::string makeHash(std::string const& fileName)
    {
        using namespace CryptoPP;

        SHA256 hash;
        std::string hashString;
        FileSource(
            fileName.c_str(),
            true,
            new HashFilter(
                hash,
                new HexEncoder(
                    new StringSink(hashString),
                    true
                )
            )
        );

        return hashString;
    }
//---------------------------------------------------------------------------------------------------------------------
    bool checkMask(std::string const& p, std::string const& mask)
    {
        std::function <bool(const char*, const char*)> match;

        match = [&match](char const *needle, char const *haystack) -> bool
        {
            for (; *needle != '\0'; ++needle)
            {
                switch (*needle)
                {
                    case '?':
                        if (*haystack == '\0')
                            return false;
                        ++haystack;
                        break;
                    case '*':
                    {
                        if (needle[1] == '\0')
                            return true;
                        size_t max = strlen(haystack);
                        for (size_t i = 0; i < max; i++)
                            if (match(needle + 1, haystack + i))
                                return true;
                        return false;
                    }
                    default:
                        if (*haystack != *needle)
                            return false;
                        ++haystack;
                }
            }
            return *haystack == '\0';
        };

        return match(mask.c_str(), p.c_str());
    }
//#####################################################################################################################
    DirectoryListing makeListing(std::string const& root, bool directories, std::string const& globber)
    {
        DirectoryListing result;
        result.root = root;
        result.filter = globber;

        using namespace boost::filesystem;
        recursive_directory_iterator iter(root), end;
        for (; iter != end; ++iter)
        {
            if (!directories && !is_regular_file(iter->status()))
                continue;
            else if (directories && !is_directory(iter->status()))
                continue;

            auto entry = relative(iter->path(), root).string();
            std::replace(entry.begin(), entry.end(), '\\', '/');

            if (!globber.empty() && !checkMask(entry, globber))
                continue;

            if (!directories)
                result.entriesWithHash.emplace_back(
                    entry,
                    makeHash(iter->path().string())
                );
            else
                result.entriesWithHash.emplace_back(
                    entry,
                    ""
                );
        }

        return result;
    }
//#####################################################################################################################
}
