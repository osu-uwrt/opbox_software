#pragma once

#include "opboxbase.hpp"
#include "opboxlogging.hpp"
#include <mutex>
#include <chrono>
#include <vector>
#include <thread>
#include <functional>

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
        bool appendToFile(const std::string& data);
        bool replaceFile(const std::string& data);

        private:
        const std::string path;
    };


    template<typename T>
    using ActuatorPattern = std::vector<std::pair<T, std::chrono::milliseconds>>;


    template<typename T>
    class IOActuator : public OutFile
    {
        public:
        IOActuator(const std::string& path, T initialState)
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


        T state() const
        {
            return _state;
        }


        void setPattern(ActuatorPattern<T> pattern)
        {
            _memberMutex.lock();
            _activePattern = pattern;
            _memberMutex.unlock();
        }

        private:
        void waitUntilTimeoutOrThreadKilled(std::chrono::milliseconds ms)
        {
            auto start = std::chrono::high_resolution_clock::now();
            
            while(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now() - start) <= ms

                && _threadRunning)
            {
                std::this_thread::sleep_for(5ms);
            }
        }


        void threadFunc()
        {
            OPBOX_LOG_DEBUG("Thread starting (%s)", _path);

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
                    auto step = _activePattern.at(i);
                    _memberMutex.unlock();
                    
                    setState(step.first);
                    waitUntilTimeoutOrThreadKilled(step.second);
                }
            }

            setState(_defaultState);
            OPBOX_LOG_DEBUG("Thread ending (%s)", _path);
        }

        void setState(T newState)
        {
            OPBOX_LOG_DEBUG("Setting state of (%s) to %d", _path, newState);
            _state = newState;
            _memberMutex.lock();
            replaceFile(std::to_string(_state));
            _memberMutex.unlock();
        }

        const std::string   _path;
        const T             _defaultState;
        bool                _threadRunning;
        std::mutex          _memberMutex; // mutex for all member variables
        std::thread         _thread;
        ActuatorPattern<T>  _activePattern;
        T                   _state;
    };
}