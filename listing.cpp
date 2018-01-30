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
    bool checkMask(boost::filesystem::path const& p, std::string const& mask)
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

        return match(mask.c_str(), p.string().c_str());
    }
//#####################################################################################################################
    DirectoryListing makeListing(std::string const& root, std::string const& globber)
    {
        DirectoryListing result;
        result.root = root;
        result.filter = globber;

        using namespace boost::filesystem;
        recursive_directory_iterator iter(root), end;
        for (; iter != end; ++iter)
        {
            if (!is_regular_file(iter->status()))
                continue;

            if (!globber.empty() && !checkMask(iter->path(), globber))
                continue;

            auto file = relative(iter->path(), root).string();
            std::replace(file.begin(), file.end(), '\\', '/');
            result.filesWithHash.emplace_back(
                file,
                makeHash(iter->path().string())
            );
        }

        return result;
    }
//#####################################################################################################################
}
