#include <serial_library/serial_library.hpp>
#include "opbox_software/opboxio.hpp"
#include "opbox_software/opboxlogging.hpp"

#define KS_PIN 0
#define KS_RED_LED 1
#define KS_YELLOW_LED 2
#define KS_GREEN_LED 3

using namespace std::placeholders;

enum KillSwitchState
{
    UNKILLED,
    KILLED
};

namespace opbox
{

    class Opbox
    {
        public:
        Opbox()
         : ksLeds(KS_GREEN_LED, KS_YELLOW_LED, KS_RED_LED),
           killSwitch(KS_PIN, std::bind(&Opbox::killSwitchStateChanged, this, _1))
        {
            //initialize serial library
        }

        int loop()
        {
            //use ios to indicate program coming up
            buzzer.setState(IOBuzzerState::IO_BUZZER_CHIRP_TWICE);
            buzzer.setNextState(IOBuzzerState::IO_BUZZER_OFF, 1s);
            usrLed.setState(IOLedState::IO_LED_BLINK_TWICE);
            usrLed.setNextState(IOLedState::IO_LED_OFF, 1s);
            ksLeds.setAllStates(IOLedState::IO_LED_SLOW_BLINK);
            return 0;
        }

        void killSwitchStateChanged(int newKillSwitchState)
        {
            OPBOX_LOG_INFO("Kill switch state changed to %d", newKillSwitchState);
        }

        private:
        //IO
        IOBuzzer buzzer;
        IOUsrLed usrLed;
        KillSwitchLeds ksLeds;
        IOGpioSensor<int> killSwitch;

        //communication
        std::unique_ptr<serial_library::SerialProcessor> serial;
    };

}

int main(int argc, char **argv)
{
    std::unique_ptr<opbox::Opbox> opbox = std::make_unique<opbox::Opbox>();
    return opbox->loop();
}
