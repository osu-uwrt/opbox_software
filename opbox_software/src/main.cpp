#include "opbox_software/opboxio.hpp"
#include "opbox_software/opboxlogging.hpp"

int main(int argc, char **argv)
{
    opbox::IOBuzzer buzzer;
    buzzer.setState(opbox::IOBuzzerState::IO_BUZZER_PANIC);

    opbox::IOLed led;
    led.setState(opbox::IOLedState::IO_LED_BLINK_TWICE);

    std::this_thread::sleep_for(2s);
    
    return 0;
}
