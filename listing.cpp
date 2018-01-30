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
//#####################################################################################################################
    DirectoryListing makeListing(std::string const& root, std::string const& extensionWhiteSelect)
    {
        DirectoryListing result;
        result.root = root;
        result.filter = extensionWhiteSelect;

        using namespace boost::filesystem;
        recursive_directory_iterator iter(root), end;
        for (; iter != end; ++iter)
        {
            if (!is_regular_file(iter->status()))
                continue;

            if (!extensionWhiteSelect.empty() && iter->path().extension().string() != extensionWhiteSelect)
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
