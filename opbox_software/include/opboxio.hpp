#pragma once

#include "opboxbase.hpp"
#include <mutex>
#include <chrono>
#include <vector>
#include <thread>

/**
 * Opbox IO interfaces
 */


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
    typename std::vector<std::pair<T, std::chrono::milliseconds>> ActuatorPattern;


    template<typename T>
    class IOActuator : public OutFile
    {
        public:
        IOActuator(const std::string& path, T initialState)
         : OutFile(path),
           _state(initialState)
        {
            replaceFile(std::to_string(initialState));
        }


        T state() const
        {
            return _state;
        }



        private:
        void setState(T newState)
        {
            _state = newState;
            replaceFile(std::to_string(_state));
        }

        T _state;
    };
}