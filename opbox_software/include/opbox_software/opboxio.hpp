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

/**
 * Opbox IO interfaces
 */

using namespace std::chrono_literals;

namespace opbox {

    /**
     * For reading short files for sensing purposes
     */
    class InFile
    {
        public:
        InFile(const std::string& path);
        bool exists() const;
        std::string readFile();

        private:
        const std::string path;
    };

    /**
     * For writing short files for sensing purposes
     */
    class OutFile
    {
        public:
        OutFile(const std::string& path);
        void appendToFile(const std::string& data);
        void replaceFile(const std::string& data);

        private:
        const std::string path;
    };

    template<typename T>
    using ActuatorInterval = std::pair<T, std::chrono::milliseconds>;

    template<typename T>
    using ActuatorPattern = std::vector<ActuatorInterval<T>>;

    template<typename T>
    using ActuatorQueue = std::deque<ActuatorInterval<ActuatorPattern<T>>>;

    template<typename T>
    class IOActuator : public OutFile
    {
        public:
        IOActuator(const std::string& path, const T& initialState)
         : OutFile(path),
           _path(path),
           _defaultState(initialState),
           _state(initialState),
           _threadRunning(true),
           _thread(std::bind(&IOActuator::threadFunc, this))
        {
            replaceFile(std::to_string(initialState));
        }


        ~IOActuator()
        {
            _threadRunning = false;
            _thread.join();
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
            auto elapsed = now - _queuePatternStartTime;
            _memberMutex.lock();
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
            OPBOX_LOG_DEBUG("Thread starting (%s)", _path.c_str());

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
            OPBOX_LOG_DEBUG("Thread ending (%s)", _path.c_str());
        }

        void setState(const T& newState)
        {
            OPBOX_LOG_DEBUG("Setting state of (%s) to %d", _path.c_str(), newState);
            _memberMutex.lock();
            _state = newState;
            std::string strState = std::to_string(_state);
            _memberMutex.unlock();
            replaceFile(strState);
        }

        const std::string   _path;
        const T             _defaultState;
        bool                _threadRunning;
        std::mutex          _memberMutex; // mutex for all member variables
        std::thread         _thread;
        ActuatorPattern<T>  _activePattern;
        ActuatorQueue<T>    _queue;
        T                   _state;
        
        std::chrono::time_point<std::chrono::system_clock> _queuePatternStartTime;
    };


    template<typename StateType, typename IOType>
    class IOController
    {
        public:
        IOController(const IOType& defaultState, const std::string& primaryPath, const std::string& backupPath = "", bool forceBackup = false)
        {
            std::string targetPath = backupPath;
            if(!forceBackup)
            {
                InFile f(primaryPath);
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

            _actuator = std::make_unique<IOActuator<IOType>>(targetPath, defaultState);
        }

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
        IO_LED_BLINK_TWICE,
        IO_LED_BLINKING
    };

    class IOLed : public IOController<IOLedState, int>
    {
        public:
        IOLed(bool forceBackup = false);

        protected:
        ActuatorPattern<int> getStatePattern(const IOLedState& state) const override;
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
}
