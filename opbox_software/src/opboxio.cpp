#include "opbox_software/opboxio.hpp"
#include <fstream>
#include <sstream>

namespace opbox {

    //
    // InFile implementation
    //

    InFile::InFile(const std::string& path)
     : path(path) { }


    bool InFile::exists() const 
    {
        std::ifstream f(path);
        bool e = f.good();
        f.close();
        return e;
    }


    std::string InFile::readFile()
    {
        std::ifstream f(path);
        std::stringstream s;
        s << f.rdbuf();
        f.close();
        return s.str();
    }

    //
    // OutFile
    //

    OutFile::OutFile(const std::string& path)
     : path(path) { }


    void OutFile::appendToFile(const std::string& data)
    {
        std::ofstream f(path, std::ofstream::app);
        f << data;
        f.close();
    }


    void OutFile::replaceFile(const std::string& data)
    {
        std::ofstream f(path, std::ofstream::trunc);
        f << data;
        f.close();
    }

    //
    // IOLed
    //

    IOLed::IOLed(bool forceBackup)
    {
        std::string targetFile = OPBOX_IO_BACKUP_LED_FILE;
        if(!forceBackup)
        {
            // test existence of default file. If none exists, fall back to the backup
            InFile f(OPBOX_IO_PRIMARY_LED_FILE);
            if(f.exists())
            {
                targetFile = OPBOX_IO_PRIMARY_LED_FILE;
            }
        }

        _led = std::make_shared<IOActuator<int>>(targetFile, 0);
    }


    void IOLed::setState(IOLedState state)
    {
        ActuatorPattern<int> patt = {{0, 1s}};

        switch(state)
        {
            case IO_LED_OFF:
                break;
            case IO_LED_ON:
                patt = {{1, 1s}};
                break;
            case IO_LED_BLINK_TWICE:
                patt = {
                    {0, 125ms},
                    {1, 125ms},
                    {0, 125ms},
                    {1, 125ms},
                    {0, 24h}
                };
                break;
            case IO_LED_BLINKING:
                patt = {
                    {1, 250ms},
                    {0, 250ms},
                    {1, 250ms},
                    {0, 250ms}
                };
                break;
            default:
                OPBOX_LOG_ERROR("IOLed state %d is not supported yet!", state);
        }

        _led->setPattern(patt);
    }

    //
    // IOBuzzer
    //

    IOBuzzer::IOBuzzer(bool forceBackup)
    {
        std::string targetFile = OPBOX_IO_BACKUP_BUZZER_FILE;
        if(!forceBackup)
        {
            InFile f(OPBOX_IO_PRIMARY_BUZZER_FILE);
            if(f.exists())
            {
                targetFile = OPBOX_IO_PRIMARY_BUZZER_FILE;
            }
        }

        _buzzer = std::make_shared<IOActuator<int>>(targetFile, 0);
    }


    IOBuzzer::~IOBuzzer()
    {
        _buzzer.reset();
    }


    void IOBuzzer::setState(IOBuzzerState state)
    {
        ActuatorPattern<int> patt = {{0, 1s}};

        switch(state)
        {
            case IO_BUZZER_OFF:
                break;
            case IO_BUZZER_CHIRP:
                patt = {
                    {1, 125ms},
                    {0, 24h}
                };
                break;
            case IO_BUZZER_CHIRP_TWICE:
                patt = {
                    {0, 125ms},
                    {1, 125ms},
                    {0, 125ms},
                    {1, 125ms},
                    {0, 24h}
                };
                break;
            case IO_BUZZER_PANIC:
                patt = {{1, 500ms}};
                for(int i = 0; i < 50; i++)
                {
                    patt.push_back({0, 5ms});
                    patt.push_back({1, 5ms});
                }
                break;
            default:
                OPBOX_LOG_ERROR("IOBuzzer state %d is not supported yet!", state);
        };

        _buzzer->setPattern(patt);
    }
}
