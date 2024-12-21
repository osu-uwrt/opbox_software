#pragma once

#include "opboxbase.hpp"
#include <list>

namespace opbox
{
    struct AlertSettings
    {
        bool
            buzzerEnabled = false,
            screenPopupEnabled = true;
        int screenPopupLifetime = 15;
    };

    struct OpboxSettings
    {
        std::list<std::string> clients;
        int
            clientPort = 0,
            diagServerPort = 0;
        bool useCustomDiagServerIp = false;
        std::string customDiagServerIp = "";
        AlertSettings
            errorAlertSettings,
            warningAlertSettings;
    };

    bool readOpboxSettingsFromFile(const std::string& path, OpboxSettings& settings);
    OpboxSettings readOpboxSettings(void);
    void writeOpboxSettingOverrides(const OpboxSettings& settings);
}
