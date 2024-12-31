#pragma once

#include "opboxbase.hpp"
#include <list>

namespace opbox
{
    //project asset management
    std::string resolveInstallPath(const std::string& installPath);
    std::string resolveAssetPath(const std::string& assetPath);
}
