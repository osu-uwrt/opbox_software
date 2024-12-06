#include "opboxio.hpp"
#include "opboxlogging.hpp"

int main(int argc, char **argv)
{
    opbox::IOActuator<int> act("test_files/c.txt", 0);
    opbox::ActuatorPattern<int> patt = {
        {1, 750ms},
        {0, 250ms}
    };

    act.setPattern(patt);
    std::this_thread::sleep_for(5s);
    
    return 0;
}
