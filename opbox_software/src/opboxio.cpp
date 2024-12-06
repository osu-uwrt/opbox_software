#include "opboxio.hpp"
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


    bool OutFile::appendToFile(const std::string& data)
    {
        std::ofstream f(path, std::ofstream::app);
        f << data;
        f.close();
    }


    bool OutFile::replaceFile(const std::string& data)
    {
        std::ofstream f(path, std::ofstream::trunc);
        f << data;
        f.close();
    }

}
