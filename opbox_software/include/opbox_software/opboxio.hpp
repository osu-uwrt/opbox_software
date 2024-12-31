#pragma once

#include "opboxbase.hpp"
#include "opboxlogging.hpp"
#include <mutex>
#include <chrono>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <memory>
#include <fstream>
#include <sstream>

/**
 * Opbox IO interfaces
 */

using namespace std::chrono_literals;

namespace opbox {
    
    //
    // Random utility function
    //
    std::string resolveInstallPath(const std::string& installPath);
    std::string resolveAssetPath(const std::string& assetPath);
    std::string resolveProgramPath(const std::string& programPath);

    class NamedObject
    {
        public:
        virtual std::string name() const = 0;
    };

    template<typename T>
    class GenericInput : public NamedObject
    {
        public:
        virtual T read() = 0;
    };

    template<typename T>
    class GenericOutput : public NamedObject
    {
        public:
        virtual void write(const T& t) = 0;
    };

    template<typename T>
    using StringConversionFunc = std::function<T(const std::string&)>;

    /**
     * For reading short files for sensing purposes
     */
    template<typename T>
    class InFile : public GenericInput<T>
    {
        public:
        InFile(const std::string& path, StringConversionFunc<T> func)
         : cvtToString(func),
           path(path) { }


        bool exists() const 
        {
            std::ifstream f(path);
            bool e = f.good();
            f.close();
            return e;
        }

        std::string name() const override
        {
            return path;
        }

        T read() override
        {
            std::ifstream f(path);
            std::stringstream s;
            s << f.rdbuf();
            f.close();
            return cvtToString(s.str());
        }

        protected:
        const std::string path;
        StringConversionFunc<T> cvtToString;
    };

    static std::string cvtStringToStringFxn(const std::string& s)
    {
        return s;
    }

    class StringInFile : public InFile<std::string>
    {
        public:
        StringInFile(const std::string& path)
         : InFile<std::string>(path, cvtStringToStringFxn) { }
    };

    /**
     * For writing short files for sensing purposes
     */
    template<typename T>
    class OutFile : public GenericOutput<T>
    {
        public:
        OutFile(const std::string& path)
         : path(path) { }


        std::string name() const
        {
            return path;
        }


        void append(const T& data)
        {
            std::ofstream f(path, std::ofstream::app);
            f << data;
            f.close();
        }


        void write(const T& data) override
        {
            std::ofstream f(path, std::ofstream::trunc);
            f << data;
            f.close();
        }

        private:
        const std::string path;
    };


    class StringOutFile : public OutFile<std::string>
    {
        public:
        StringOutFile(const std::string& path)
         : OutFile<std::string>(path) { }
    };

    // #if defined ()     use to detect cmake flag indicating pigpio library present
    // #include <pigpio.h>
    // #define USE_REAL_GPIO 1
    // #else
    #define USE_REAL_GPIO 0
    // #endif

    enum GPIODirection
    {
        INPUT,
        OUTPUT
    };

    enum GPIOState
    {
        LOW,
        HIGH
    };

    enum GPIOPullUpDown
    {
        PULLUP,
        PULLDOWN,
        NONE
    };

    class GPIO
    {
        public:
        static bool initialize();
        static bool isInitialized();

        GPIO(const short pin,
            GPIODirection direction,
            GPIOPullUpDown pud = GPIOPullUpDown::NONE,
            GPIOState initialState = GPIOState::LOW,
            bool forceFakeGpio = false,
            const std::string& fakeGpioFile = "./test_gpio");

        const short pin() const;
        const GPIOPullUpDown getPud() const;
        const GPIODirection direction() const;
        GPIOState state();
        void setState(const GPIOState& state);

        protected:
        static bool _initialized;
        const short _pin;
        const GPIODirection _direction;
        const GPIOPullUpDown _pud;
        const bool _forceFakeGpio;
        const std::string _fakeGpioFile;
        GPIOState _state;
    };

    class GPIOError : public std::runtime_error
    {
        public:
        GPIOError(const char* what, const GPIO& gpio)
         : std::runtime_error(what),
           gpio(gpio) { }
        
        const GPIO offender()
        {
            return gpio;
        }

        private:
        const GPIO& gpio;
    };


    class GPIOInput : public GenericInput<int>, public GPIO
    {
        public:
        GPIOInput(const short pin,
                bool forceFakeGpio = false,
                const std::string& fakeGpioFile = "./test_gpio")
         : GPIO(pin, GPIODirection::INPUT, GPIOPullUpDown::PULLUP, GPIOState::LOW, forceFakeGpio, fakeGpioFile) { }
        
        std::string name() const override;
        int read() override;
    };


    class GPIOOutput : public GenericOutput<int>, public GPIO
    {
        public:
        GPIOOutput(const short pin,
                bool forceFakeGpio = false,
                const std::string& fakeGpioFile = "./test_gpio")
         : GPIO(pin, GPIODirection::OUTPUT, GPIOPullUpDown::NONE, GPIOState::LOW, forceFakeGpio, fakeGpioFile) { }
        
        std::string name() const override;
        void write(const int& s) override;
    };


    template<typename T>
    using ActuatorInterval = std::pair<T, std::chrono::milliseconds>;

    template<typename T>
    using ActuatorPattern = std::vector<ActuatorInterval<T>>;

    template<typename T>
    using ActuatorQueue = std::deque<ActuatorInterval<ActuatorPattern<T>>>;

    template<typename T>
    class IOActuator
    {
        public:
        IOActuator(std::unique_ptr<GenericOutput<T>> output, const T& initialState)
         : _defaultState(initialState),
           _state(initialState),
           _threadRunning(true),
           _output(std::move(output))
        {
            _output->write(initialState);
            _thread = std::make_unique<std::thread>(std::bind(&IOActuator::threadFunc, this));
        }


        ~IOActuator()
        {
            _memberMutex.lock();
            _threadRunning = false;
            _memberMutex.unlock();
            _thread->join();
        }


        T state()
        {
            _memberMutex.lock();
            T ret = _state;
            _memberMutex.unlock();
            return ret;
        }


        void setPattern(const ActuatorPattern<T>& pattern, bool clearQueue = true)
        {
            _memberMutex.lock();
            if(clearQueue)
            {
                OPBOX_LOG_DEBUG("IOActuator clearing queue to override current pattern");

                //interrupt queue
                _queue.clear();
            } else
            {
                //to ensure proper timing, subtract elapsed time from current front item of queue
                _queue.front().second -= std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock().now() - _queuePatternStartTime);
            }
            _queue.push_front({pattern, 0s});
            _memberMutex.unlock();
        }

        
        void setNextPattern(const ActuatorPattern<T>& pattern, const std::chrono::milliseconds& delay)
        {
            _memberMutex.lock();
            _queue.push_back({pattern, delay});
            _memberMutex.unlock();
        }

        private:
        bool isQueueDelayUp()
        {
            std::chrono::time_point now = std::chrono::system_clock::now();
            _memberMutex.lock();
            auto elapsed = now - _queuePatternStartTime;
            bool ret = !_queue.empty()
                        && elapsed >= _queue.front().second;
            _memberMutex.unlock();
            return ret;
        }

        bool waitUntilTimeoutOrThreadKilled(const std::chrono::milliseconds& ms)
        {
            auto start = std::chrono::system_clock::now();
            bool qdUp = isQueueDelayUp();
            
            while(std::chrono::system_clock::now() - start <= ms 
                && !qdUp
                && _threadRunning)
            {
                std::this_thread::sleep_for(5ms);
                qdUp = isQueueDelayUp();
            }

            return qdUp;
        }


        void threadFunc()
        {
            _queuePatternStartTime = std::chrono::system_clock::now();
            OPBOX_LOG_DEBUG("Thread starting (%s)", _output->name().c_str());

            while(_threadRunning)
            {
                _memberMutex.lock();
                size_t activePatternSize = _activePattern.size(); 
                _memberMutex.unlock();

                if(activePatternSize > 0)
                {
                    std::this_thread::sleep_for(5ms);
                }

                for(int i = 0; i < activePatternSize; i++)
                {
                    _memberMutex.lock();
                    ActuatorInterval<T> step = {_defaultState, 100ms};
                    try
                    {
                        step = _activePattern.at(i);
                    } catch(std::out_of_range& ex) {
                        OPBOX_LOG_DEBUG("IOActuator pattern out of range, ignoring");

                    }
                    _memberMutex.unlock();
                    
                    setState(step.first);
                    if(waitUntilTimeoutOrThreadKilled(step.second))
                    {
                        break;
                    }
                }

                if(isQueueDelayUp())
                {
                    _memberMutex.lock();
                    size_t queueSize = _queue.size();
                    if(queueSize > 0)
                    {
                        _activePattern = _queue.front().first;
                        _queue.pop_front();
                        _queuePatternStartTime = std::chrono::system_clock::now();
                    }

                    _memberMutex.unlock();
                }
            }

            setState(_defaultState);
            OPBOX_LOG_DEBUG("Thread ending (%s)", _output->name().c_str());
        }

        void setState(const T& newState)
        {
            OPBOX_LOG_DEBUG("Setting state of (%s) to %d", _output->name().c_str(), newState);
            _memberMutex.lock();
            _state = newState;
            _output->write(_state);
            _memberMutex.unlock();
        }

        const T             _defaultState;
        bool                _threadRunning;
        std::mutex          _memberMutex; // mutex for all member variables
        ActuatorPattern<T>  _activePattern;
        ActuatorQueue<T>    _queue;
        T                   _state;
        
        std::unique_ptr<std::thread>      _thread;
        std::unique_ptr<GenericOutput<T>> _output;
        std::chrono::time_point<std::chrono::system_clock> _queuePatternStartTime;
    };

    template<typename T>
    using InputStateChangedCallback = std::function<void(const T&)>; 

    template<typename T>
    class IOSensor
    {
        public:
        IOSensor(std::unique_ptr<GenericInput<T>> input, InputStateChangedCallback<T> onStateChange)
         : _threadRunning(true),
           _input(std::move(input)),
           _onStateChange(onStateChange),
           _thread(std::bind(&IOSensor::threadFunc, this)) { }
        
        ~IOSensor()
        {
            _memberMutex.lock();
            _threadRunning = false;
            _memberMutex.unlock();
            _thread.join();
        }

        T state()
        {
            _memberMutex.lock();
            T s = _input->read();
            _memberMutex.unlock();
            return s;
        }

        private:
        void threadFunc()
        {
            _memberMutex.lock();
            T state = _input->read(),
            newState = state;
            _memberMutex.unlock();

            while(_threadRunning)
            {
                _memberMutex.lock();
                newState = _input->read();
                _memberMutex.unlock();

                if(newState != state)
                {
                    _onStateChange(newState);
                }

                state = newState;
            }
        }

        bool                                _threadRunning;
        std::mutex                          _memberMutex;
        std::thread                         _thread;
        std::unique_ptr<GenericInput<T>>    _input;
        InputStateChangedCallback<T>        _onStateChange;
    };


    template<typename StateType, typename IOType>
    class IOController
    {
        public:

        template<typename T>
        static std::unique_ptr<IOActuator<T>> makeFileActuator(const T& defaultState, const std::string& primaryPath, const std::string& backupPath, bool forceBackup = false)
        {
            std::string targetPath = backupPath;
            if(!forceBackup)
            {
                StringInFile f(primaryPath);
                if(f.exists())
                {
                    targetPath = primaryPath;
                } else
                {
                    OPBOX_LOG_DEBUG("IOController primary path %s not found, falling back to backup path %s", primaryPath.c_str(), backupPath.c_str());
                }
            } else 
            {
                OPBOX_LOG_DEBUG("IOController overriding primary path %s for backup path %s", primaryPath.c_str(), backupPath.c_str());
            }

            return std::make_unique<IOActuator<T>>(std::make_unique<OutFile<T>>(targetPath), defaultState);
        }

        IOController(std::unique_ptr<IOActuator<IOType>> actuator)
         : _actuator(std::move(actuator)) { }

        void setState(const StateType& state, bool clearQueue = true)
        {
            _actuator->setPattern(getStatePattern(state), clearQueue);
        }

        void setNextState(const StateType& state, const std::chrono::milliseconds& delay)
        {
            _actuator->setNextPattern(getStatePattern(state), delay);
        }

        protected:
        virtual ActuatorPattern<IOType> getStatePattern(const StateType& state) const = 0;

        private:
        std::unique_ptr<IOActuator<IOType>> _actuator;
    };


    enum IOLedState
    {
        IO_LED_OFF,
        IO_LED_ON,
        IO_LED_BLINK_ONCE,
        IO_LED_BLINK_TWICE,
        IO_LED_FAST_BLINK,
        IO_LED_SLOW_BLINK
    };


    class IOLed : public IOController<IOLedState, int>
    {
        public:
        IOLed(std::unique_ptr<IOActuator<int>> actuator)
         : IOController<IOLedState, int>(std::move(actuator)) { }

        protected:
        ActuatorPattern<int> getStatePattern(const IOLedState& state) const override;
    };


    class IOUsrLed : public IOLed
    {
        public:
        IOUsrLed(bool forceBackup = false)
         : IOLed(makeFileActuator(
            0, 
            resolveAssetPath(OPBOX_IO_PRIMARY_LED_FILE), 
            resolveAssetPath(OPBOX_IO_BACKUP_LED_FILE), 
            forceBackup)) { }
    };


    class IOGpioLed : public IOLed
    {
        public:
        IOGpioLed(int pin, 
                GPIOState defaultState = GPIOState::LOW,
                bool forceFakeGpio = false,
                const std::string& fakeGpioFile = "./test_gpio")
         : IOLed(std::make_unique<IOActuator<int>>(std::make_unique<GPIOOutput>(pin, forceFakeGpio, fakeGpioFile), defaultState == GPIOState::HIGH)) { }
    };


    class KillSwitchLeds
    {
        public:
        KillSwitchLeds(int greenPin, int yellowPin, int redPin, bool forceFakeGpio = false);
        void setAllStates(IOLedState state);
        void setRedState(IOLedState state);
        void setYellowState(IOLedState state);
        void setGreenState(IOLedState state);

        private:
        IOGpioLed 
            _green,
            _yellow,
            _red;
    };


    enum IOBuzzerState
    {
        IO_BUZZER_OFF,
        IO_BUZZER_CHIRP,
        IO_BUZZER_CHIRP_TWICE,
        IO_BUZZER_PANIC
    };

    class IOBuzzer : public IOController<IOBuzzerState, int>
    {
        public:
        IOBuzzer(bool forceBackup = false);

        protected:
        ActuatorPattern<int> getStatePattern(const IOBuzzerState& state) const override;
    };


    template<typename T>
    class IOGpioSensor : public IOSensor<T>
    {
        public:
        IOGpioSensor(
            int pin,
            InputStateChangedCallback<T> callback,
            bool forceFakeGpio = false,
            const std::string& fakeGpioFile = "./test_gpio")
         : IOSensor<T>(std::make_unique<GPIOInput>(pin, forceFakeGpio, fakeGpioFile), callback)
         { }
    };
}
