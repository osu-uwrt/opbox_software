#include "opbox_software/opboxutil.hpp"
#include "opbox_software/opboxlogging.hpp"
#include "opbox_software/opboxio.hpp"

namespace opbox
{
    std::string resolveInstallPath(const std::string& installPath)
    {
        char *prefixPath = getenv("AMENT_PREFIX_PATH");
        if(!prefixPath)
        {
            OPBOX_LOG_ERROR("Env var AMENT_PREFIX_PATH not found!");
            return installPath;
        }

        std::string prefixPathStr = prefixPath;

        size_t previousPos = 0;
        for(size_t pos = prefixPathStr.find(":", previousPos); 
            pos != std::string::npos; 
            pos = prefixPathStr.find(":", previousPos))
        {
            std::string path = prefixPathStr.substr(previousPos, pos - previousPos);
            size_t rpos = path.rfind('/');
            
            OPBOX_LOG_DEBUG("Get install path checking path %s", path.c_str());
            
            if(rpos != std::string::npos)
            {
                std::string package = path.substr(rpos + 1);
                OPBOX_LOG_DEBUG("Got package name for %s as %s", path.c_str(), package.c_str());
                if(package == "opbox_software")
                {
                    std::string asset = path + "/" + installPath;
                    OPBOX_LOG_DEBUG("Get install path as %s", asset.c_str());
                    return asset;
                }
            }

            previousPos = pos + 1;
        }

        OPBOX_LOG_ERROR("Unable to find install directory for opbox_software!");
        return installPath;
    }


    std::string resolveAssetPath(const std::string& assetPath)
    {
        std::string absPath = assetPath;
        if(assetPath.find("share://") == 0)
        {
            absPath = resolveInstallPath("share/opbox_software/") + assetPath.substr(sizeof("share://") - 1);
        }

        if(assetPath.find("lib://") == 0)
        {
            absPath = resolveInstallPath("lib/opbox_software/") + assetPath.substr(sizeof("lib://") - 1);
        }

        OPBOX_LOG_ERROR("Got asset %s as %s", assetPath.c_str(), absPath.c_str());
        return absPath;
    }
}