#include <csignal>
#include <serial_library/serial_library.hpp>
#include "opbox_software/opboxio.hpp"
#include "opbox_software/opboxlogging.hpp"
#include "opbox_software/opboxutil.hpp"
#include "opbox_software/opboxcomms.hpp"

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

typedef std::map<std::string, opbox::OpboxRobotLink::UniquePtr> SerialProcMap;
void signalHandler(int signum);

namespace opbox
{
    class Opbox
    {
        public:
        Opbox()
         : _loopRunning(true),
           _settings(readOpboxSettings()),
           _ksLeds(KS_GREEN_LED, KS_YELLOW_LED, KS_RED_LED)
        //    _killSwitch(KS_PIN, std::bind(&Opbox::killButtonStateChanged, this, _1))
        {
            //initialize serial library
            for(std::string client : _settings.clients)
            {
                
            }

            //install signal handler
            signal(SIGINT, &signalHandler);
        }


        int loop()
        {
            //use ios to indicate program coming up
            _buzzer.setState(IOBuzzerState::IO_BUZZER_CHIRP_TWICE);
            _buzzer.setNextState(IOBuzzerState::IO_BUZZER_OFF, 1s);
            _usrLed.setState(IOLedState::IO_LED_BLINK_TWICE);
            _usrLed.setNextState(IOLedState::IO_LED_OFF, 1s);
            _ksLeds.setAllStates(IOLedState::IO_LED_SLOW_BLINK);

            OPBOX_LOG_INFO("Opbox initialized, loop starting");
            
            bool loopRunning = true;
            do
            {
                std::this_thread::sleep_for(1ms);
                _loopRunningMutex.lock();
                loopRunning = _loopRunning;
                _loopRunningMutex.unlock();
            } while(loopRunning);

            OPBOX_LOG_INFO("Opbox loop ending");
            return 0;
        }

        
        void endLoop()
        {
            _loopRunningMutex.lock();
            _loopRunning = false;
            _loopRunningMutex.unlock();
            OPBOX_LOG_DEBUG("Requested opbox loop end");
        }
        
        private:

        void killButtonStateChanged(int newKillButtonState)
        {
            OPBOX_LOG_INFO("Kill button state changed to %d", newKillButtonState);
        }

        //loop
        std::mutex _loopRunningMutex;
        bool _loopRunning = true;

        //settings
        OpboxSettings _settings;

        //IO
        IOBuzzer _buzzer;
        IOUsrLed _usrLed;
        KillSwitchLeds _ksLeds;
        // IOGpioSensor<int> _killSwitch;

        //communication
        SerialProcMap _serialProcMap;
    };
}

std::unique_ptr<opbox::Opbox> _opbox;

void signalHandler(int signum)
{
    if(signum == SIGINT)
    {
        OPBOX_LOG_INFO("Signal SIGINT received by program");
        _opbox->endLoop();
    }
}

int main(int argc, char **argv)
{
    _opbox = std::make_unique<opbox::Opbox>();
    return _opbox->loop();
}
