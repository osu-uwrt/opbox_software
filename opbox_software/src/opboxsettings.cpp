#include "opbox_software/opboxsettings.hpp"
#include "opbox_software/opboxlogging.hpp"
#include "opbox_software/opboxio.hpp"
#include <yaml-cpp/yaml.h>

namespace opbox
{
    std::string yamlNodeTypeToString(const YAML::NodeType::value& type)
    {
        switch(type)
        {
            case YAML::NodeType::value::Map:
                return "Map";
            case YAML::NodeType::value::Null:
                return "Null";
            case YAML::NodeType::value::Scalar:
                return "Scalar";
            case YAML::NodeType::value::Sequence:
                return "Sequence";
            case YAML::NodeType::value::Undefined:
                return "Undefined";
            default:
                return "ERROR TYPE";
        }
    }

    bool getNodeFromYaml(const YAML::Node& src, const std::string& name, const YAML::NodeType::value& type, YAML::Node &dst)
    {
        if(src[name] && src[name].Type() == type)
        {
            dst = src[name];
            return true;
        } else
        {
            if(!src[name])
            {
                OPBOX_LOG_DEBUG("Could NOT find node name %s; does not exist", name.c_str());
            } else
            {
                OPBOX_LOG_DEBUG("Could NOT find node name %s in YAML of type %s. Actual type was %s", 
                    name.c_str(),
                    yamlNodeTypeToString(type),
                    yamlNodeTypeToString(src[name].Type()));
            }

            return false;
        }
    }


    bool allTrue(const std::vector<bool>& v)
    {
        bool r = true;
        for(auto it = v.begin(); it != v.end(); it++)
        {
            r = r && *it;
        }

        return r;
    }


    bool readAlertSettingsFromNode(const YAML::Node& yaml, const std::string& prefix, const bool& scrnPopLifeOptional, AlertSettings& settings)
    {
        YAML::Node node;

        std::vector<bool> results;

        results.push_back(getNodeFromYaml(yaml, prefix + "_buzzer_enabled", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.buzzerEnabled = node.as<bool>();
        }

        results.push_back(getNodeFromYaml(yaml, prefix + "_screen_popup_enabled", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.screenPopupEnabled = node.as<bool>();
        }

        results.push_back(getNodeFromYaml(yaml, prefix + "_screen_popup_lifetime", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.screenPopupLifetime = node.as<double>();
        }

        //last result (screen popup life) may be optional based on argument
        results.back() = results.back() || scrnPopLifeOptional;

        return allTrue(results);
    }


    bool readOpboxSettingsFromFile(const std::string& path, OpboxSettings& settings)
    {
        std::vector<bool> results;

        if(!StringInFile(path).exists())
        {
            OPBOX_LOG_ERROR("Opbox Settings file %s does not exist", path.c_str());
            return false;
        }

        OPBOX_LOG_DEBUG("Opening YAML file %s", path.c_str());
        YAML::Node 
            yaml = YAML::LoadFile(path)["opbox_config"],
            node;
        
        results.push_back(getNodeFromYaml(yaml, "client", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.client = node.as<std::string>();
        }

        results.push_back(getNodeFromYaml(yaml, "client_port", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.clientPort = node.as<int>();
        }

        results.push_back(getNodeFromYaml(yaml, "diag_server_port", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.diagServerPort = node.as<int>();
        }

        results.push_back(getNodeFromYaml(yaml, "use_custom_diag_server_ip", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.useCustomDiagServerIp = node.as<bool>();
        }

        results.push_back(getNodeFromYaml(yaml, "custom_diag_server_ip", YAML::NodeType::value::Scalar, node));
        if(results.back())
        {
            settings.customDiagServerIp = node.as<std::string>();
        }

        results.push_back(readAlertSettingsFromNode(yaml, "error", true, settings.errorAlertSettings));
        results.push_back(readAlertSettingsFromNode(yaml, "warn", false, settings.warningAlertSettings));
        
        return allTrue(results);
    }


    OpboxSettings readOpboxSettings(void)
    {
        OpboxSettings s;

        //read default settings
        std::string settingsFile = resolveAssetPath("share://config/opbox_config.yaml");
        if(!readOpboxSettingsFromFile(settingsFile, s))
        {
            OPBOX_LOG_ERROR("Some settings unreadable from settings file %s", settingsFile.c_str());
        }

        //read overrides
        try
        {
            std::string overridesFile = resolveAssetPath("share://config_override/opbox_config_override.yaml");
            readOpboxSettingsFromFile(overridesFile, s);
        } catch(std::runtime_error&)
        {
            OPBOX_LOG_INFO("Skipping setting overrides file because none was found.");
        }

        return s;
    }


    void writeOpboxSettingOverrides(const OpboxSettings& settings)
    {
        OPBOX_LOG_ERROR("Opbox setting overrides are not supported yet");
    }
}
