#include "opbox_software/opboxio.hpp"

namespace opbox {

    //
    // IOLed
    //

    ActuatorPattern<int> IOLed::getStatePattern(const IOLedState& state) const
    {
        ActuatorPattern<int> patt = {{0, 1s}};

        switch(state)
        {
            case IO_LED_OFF:
                break;
            case IO_LED_ON:
                patt = {{1, 1s}};
                break;
            case IO_LED_BLINK_ONCE:
                patt = {
                    {0, 125ms},
                    {1, 125ms},
                    {0, 125ms},
                    {0, 24h}
                };
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
            case IO_LED_FAST_BLINK:
                patt = {
                    {0, 250ms},
                    {1, 250ms},
                };
                break;
            case IO_LED_SLOW_BLINK:
                patt = {
                    {0, 1s},
                    {1, 1s}
                };
                break;
            default:
                OPBOX_LOG_ERROR("IOLed state %d is not supported yet!", state);
        }

        return patt;
    }

    //
    // IOBuzzer
    //

    IOBuzzer::IOBuzzer(bool forceBackup)
     : IOController(makeFileActuator<int>(
        0, 
        opbox::resolveAssetPath(OPBOX_IO_PRIMARY_BUZZER_FILE), 
        opbox::resolveAssetPath(OPBOX_IO_BACKUP_BUZZER_FILE), 
        forceBackup)) { }


    ActuatorPattern<int> IOBuzzer::getStatePattern(const IOBuzzerState& state) const
    {
        ActuatorPattern<int> patt = {{0, 1s}};

        switch(state)
        {
            case IO_BUZZER_OFF:
                break;
            case IO_BUZZER_LONG_CHIRP:
                patt = {
                    {1, 500ms},
                    {0, 24h}
                };
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

        return patt;
    }

    //
    // GPIO static functions
    //

    bool GPIO::_initialized = false;

    bool GPIO::initialize()
    {
        #if USE_REAL_GPIO
        int res = gpioInitialise()
        if(res < 0)
        {
            OPBOX_LOG_ERROR("GPIO initialization failed: %d", res);
            _initialized = false;
            return false;
        }

        OPBOX_LOG_DEBUG("GPIO initialized successfully");
        _initialized = true;
        #endif

        return true;
    }


    bool GPIO::isInitialized()
    {
        return _initialized;
    }

    //
    // GPIO instance functions
    //

    GPIO::GPIO(const short pin,
        GPIODirection direction, 
        GPIOPullUpDown pud,
        GPIOState initialState,
        bool forceFakeGpio,
        const std::string& fakeGpioFile)
     : _pin(pin),
       _pud(pud),
       _direction(direction),
       _forceFakeGpio(forceFakeGpio),
       _fakeGpioFile(fakeGpioFile) { }


    const short GPIO::pin() const
    {
        return _pin;
    }


    const GPIOPullUpDown GPIO::getPud() const
    {
        return _pud;
    }


    const GPIODirection GPIO::direction() const
    {
        return _direction;
    }


    GPIOState GPIO::state()
    {
        if(direction() == GPIODirection::OUTPUT)
        {
            return _state;
        }

        if(_forceFakeGpio || !(USE_REAL_GPIO))
        {
            StringInFile in(_fakeGpioFile);
            if(!in.exists())
            {
                return GPIOState::LOW;
            }

            return (GPIOState) std::stoi(in.read());
        }

        #if USE_REAL_GPIO
        int res = gpioRead(pin());
        if(res < 0)
        {
            OPBOX_LOG_ERROR("gpioRead failed: %d", res);
        }
        #endif
    }


    void GPIO::setState(const GPIOState& state)
    {
        if(direction() == GPIODirection::INPUT)
        {
            throw GPIOError("Cannot set the state of an input", *this);
        }

        OPBOX_LOG_DEBUG("Setting state of GPIO %d to %d", pin(), state);

        if(_forceFakeGpio || !(USE_REAL_GPIO))
        {
            StringOutFile(_fakeGpioFile).write(std::to_string(state));
        }

        #if USE_REAL_GPIO
        int res = gpioWrite(pin(), PI_HIGH);
        if(res < 0)
        {
            OPBOX_LOG_ERROR("gpioWrite failed: %d", res);
        }
        #endif
        
        _state = state;
    }

    //
    // GPIOInput
    //

    std::string GPIOInput::name() const
    {
        return "GPIO Input " + std::to_string(pin());
    }


    int GPIOInput::read()
    {
        return (int) state();
    }

    //
    // GPIOOutput
    //

    std::string GPIOOutput::name() const
    {
        return "GPIO Output " + std::to_string(pin());
    }


    void GPIOOutput::write(const int& s)
    {
        setState((GPIOState) s);
    }

    //
    // KillSwitchLeds
    //
    KillSwitchLeds::KillSwitchLeds(int greenPin, int yellowPin, int redPin, bool forceFakeGpio)
     : _red(redPin, GPIOState::LOW, forceFakeGpio, resolveAssetPath("red_led")),
       _yellow(yellowPin, GPIOState::LOW, forceFakeGpio, resolveAssetPath("yellow_led")),
       _green(greenPin, GPIOState::LOW, forceFakeGpio, resolveAssetPath("green_led")) { }

    void KillSwitchLeds::setAllStates(IOLedState state)
    {
        _red.setState(state);
        _yellow.setState(state);
        _green.setState(state);
    }


    void KillSwitchLeds::setRedState(IOLedState state)
    {
        _red.setState(state);
    }


    void KillSwitchLeds::setYellowState(IOLedState state)
    {
        _yellow.setState(state);
    }


    void KillSwitchLeds::setGreenState(IOLedState state)
    {
        _green.setState(state);
    }
}
