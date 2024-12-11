#include "opbox_software/opboxio.hpp"
#include "opbox_software/opboxlogging.hpp"

int main(int argc, char **argv)
{
    opbox::IOLed led;
    led.setState(opbox::IOLedState::IO_LED_BLINKING);

    std::this_thread::sleep_for(2min);
    
    return 0;
}
