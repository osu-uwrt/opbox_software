#include <csignal>
#include "opbox_software/opboxio.hpp"
#include "opbox_software/opboxlogging.hpp"
#include "opbox_software/opboxutil.hpp"
#include "opbox_software/opboxcomms.hpp"
#include "opbox_software/opboxlinux.hpp"
#include "opbox_software/opboxsettings.hpp"

#define KS_PIN 0
#define KS_RED_LED 1
#define KS_YELLOW_LED 2
#define KS_GREEN_LED 3

using namespace std::placeholders;

void signalHandler(int signum);

namespace opbox
{
    class Opbox
    {
        public:
        Opbox()
         : _loopRunning(true),
           _settings(readOpboxSettings()),
           _ksLeds(KS_GREEN_LED, KS_YELLOW_LED, KS_RED_LED),
           _killSwitch(KS_PIN, std::bind(&Opbox::killButtonStateChanged, this, _1))
        {
            //install signal handler
            signal(SIGINT, &signalHandler);
            sendSystemNotification(NotificationType::NOTIFICATION_WARNING, "Opbox System", "Opbox Software started.");
        }


        int loop()
        {
            //use ios to indicate program coming up
            _ioMutex.lock();
            _buzzer.setState(IOBuzzerState::IO_BUZZER_CHIRP_TWICE);
            _buzzer.setNextState(IOBuzzerState::IO_BUZZER_OFF, 1s);
            _usrLed.setState(IOLedState::IO_LED_BLINK_TWICE);
            _usrLed.setNextState(IOLedState::IO_LED_OFF, 1s);
            _ksLeds.setAllStates(IOLedState::IO_LED_SLOW_BLINK);
            _ioMutex.unlock();
            bool loopRunning = true;

            OPBOX_LOG_DEBUG("Attempting to initialize robot link for client %s", _settings.client.c_str());
            try
            {
                _robotLink = std::make_unique<opbox::RobotLink>(
                    _settings.client,
                    _settings.clientPort,

                    [this] (
                        const NotificationType& type,
                        const std::string& sensor,
                        const std::string& desc)
                    {
                        this->handleNotificationFromRobot(this->_settings.client, type, sensor, desc);
                    },

                    [this] (
                        const KillSwitchState& robotKillState,
                        const ThrusterState& thrusterState,
                        const DiagnosticState& diagState,
                        const LeakState& leakState)
                    {
                        this->handleStatusFromRobot(this->_settings.client, robotKillState, thrusterState, diagState, leakState);
                    },

                    [this] (const bool& connected)
                    {
                        this->handleRobotConnectionStateChange(this->_settings.client, connected);
                    }
                );
            } catch(serial_library::FatalSerialLibraryException& ex)
            {
                OPBOX_LOG_ERROR("Failed to initialize robot link: %s", ex.what());
            }

            OPBOX_LOG_INFO("Opbox initialized, loop starting");
            
            while(loopRunning)
            {
                std::this_thread::sleep_for(250ms);
                size_t focusedClientIdx = 0;

                //update looprunning
                _loopRunningMutex.lock();
                loopRunning = _loopRunning;
                _loopRunningMutex.unlock();
            }

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

        void ioActuatorAlert(const NotificationType& type)
        {
            _ioMutex.lock();

            //actuate IOs based on notification type
            switch(type)
            {
                case NOTIFICATION_WARNING:
                    //buzzer notification
                    _buzzer.setState(IOBuzzerState::IO_BUZZER_CHIRP);
                    _buzzer.setNextState(IOBuzzerState::IO_BUZZER_OFF, 500ms);

                    //user led
                    _usrLed.setState(IOLedState::IO_LED_BLINK_ONCE);
                    _usrLed.setNextState(IOLedState::IO_LED_OFF, 500ms);
                    break;
                case NOTIFICATION_ERROR:
                    //buzzer notification
                    _buzzer.setState(IOBuzzerState::IO_BUZZER_CHIRP_TWICE);
                    _buzzer.setNextState(IOBuzzerState::IO_BUZZER_OFF, 500ms);

                    //user led
                    _usrLed.setState(IOLedState::IO_LED_BLINK_TWICE);
                    _usrLed.setNextState(IOLedState::IO_LED_SLOW_BLINK, 500ms);
                    break;
                case NOTIFICATION_FATAL:
                    //buzzer notification
                    _buzzer.setState(IOBuzzerState::IO_BUZZER_PANIC);
                    _buzzer.setNextState(IOBuzzerState::IO_BUZZER_OFF, 4s);

                    //user led
                    _usrLed.setState(IOLedState::IO_LED_FAST_BLINK);
                    break;
            }

            _ioMutex.unlock();
        }


        void killButtonStateChanged(const int& newKillButtonState)
        {
            OPBOX_LOG_INFO("Kill button state changed to %d", newKillButtonState);
            ioActuatorAlert(NOTIFICATION_WARNING);
            
            _commMutex.lock();
            if(_robotLink)
            {
                _robotLink->sendKillButtonState((KillSwitchState) newKillButtonState);
            }
            _commMutex.unlock();
        }


        void handleNotificationFromRobot(
            const std::string& robot,
            const NotificationType& type,
            const std::string& sensor,
            const std::string& desc)
        {
            //system notification
            sendSystemNotification(type,
                notificationTypeToString(type) + " from  " + robot + "(" + sensor + ")",
                desc);
            
            //window notification
            alert(type, robot + ": " + sensor + " alert", desc, "OK");

            //io notification
            ioActuatorAlert(type);
        }


        void handleStatusFromRobot(
            const std::string& robotName,
            const KillSwitchState& robotKillState,
            const ThrusterState& thrusterState,
            const DiagnosticState& diagState,
            const LeakState& leakState)
        {

        }


        void handleRobotConnectionStateChange(const std::string& robot, const bool& connected)
        {
            std::string
                verb = (connected ? "connected" : "disconnected"),
                desc = robot + " has " + verb + ".";
            
            OPBOX_LOG_INFO("%s", desc.c_str());

            sendSystemNotification(
                NotificationType::NOTIFICATION_WARNING,
                robot + " " + verb,
                desc);

            if(connected)
            {
                //stop browser process if it is running
                if(_browserProcess && _browserProcess->running())
                {
                    _browserProcess->kill();
                    _browserProcess->wait();
                }

                std::string browserHost = _settings.customDiagServerIp;
                if(!_settings.useCustomDiagServerIp)
                {
                    browserHost = robot;
                }

                //start new browswer process
                std::string url = "http://" + browserHost + ":" + std::to_string(_settings.diagServerPort);
                std::vector<std::string> args = { url, "--kiosk" }; //"kiosk" starts fullscreen
                OPBOX_LOG_DEBUG("Starting browser with url %s", url.c_str());

                _browserProcess = std::make_unique<Subprocess>(
                    "/usr/bin/sensible-browser", args);
                
                _browserProcess->run();
            }
        }

        //loop
        std::mutex _loopRunningMutex;
        bool _loopRunning = true;

        //settings
        OpboxSettings _settings;

        //diagnostics web server
        std::unique_ptr<Subprocess> _browserProcess;

        //IO
        std::mutex _ioMutex;
        IOBuzzer _buzzer;
        IOUsrLed _usrLed;
        KillSwitchLeds _ksLeds;
        IOGpioSensor<int> _killSwitch;

        //communication
        std::mutex _commMutex;
        RobotLink::UniquePtr _robotLink;
        std::string _primaryRobot;
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
    opbox::initializeNotifications();
    _opbox = std::make_unique<opbox::Opbox>();
    int retval = _opbox->loop();
    opbox::deinitializeNotifications();
    return retval;
}
