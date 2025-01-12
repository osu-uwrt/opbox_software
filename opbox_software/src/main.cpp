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

template<typename T>
using ProtectedResource = serial_library::ProtectedResource<T>;

void signalHandler(int signum);

namespace opbox
{
    class Opbox
    {
        public:
        Opbox()
         : _loopRunning(true),
           _settings(readOpboxSettings()),
           _buzzerResource(std::make_unique<IOBuzzer>()),
           _usrLedResource(std::make_unique<IOUsrLed>()),
           _ksLedsResource(std::make_unique<KillSwitchLeds>(KS_GREEN_LED, KS_YELLOW_LED, KS_RED_LED)),
           _killSwitchResource(std::make_unique<IOGpioSensor<int>>(KS_PIN, std::bind(&Opbox::killButtonStateChanged, this, _1))),
           _robotLinkResource(std::unique_ptr<RobotLink>())
        {
            //install signal handler
            signal(SIGINT, &signalHandler);
            sendSystemNotification(NotificationType::NOTIFICATION_WARNING, "Opbox System", "Opbox Software started.");
        }


        int loop()
        {
            //use ios to indicate program coming up
            std::unique_ptr<IOBuzzer> buzzer = _buzzerResource.lockResource();
            buzzer->setState(IOBuzzerState::IO_BUZZER_CHIRP_TWICE);
            buzzer->setNextState(IOBuzzerState::IO_BUZZER_OFF, 1s);
            _buzzerResource.unlockResource(std::move(buzzer));

            std::unique_ptr<IOUsrLed> usrLed = _usrLedResource.lockResource();
            usrLed->setState(IOLedState::IO_LED_BLINK_TWICE);
            usrLed->setNextState(IOLedState::IO_LED_OFF, 1s);
            _usrLedResource.unlockResource(std::move(usrLed));

            std::unique_ptr<KillSwitchLeds> ksLeds = _ksLedsResource.lockResource();
            ksLeds->setAllStates(IOLedState::IO_LED_SLOW_BLINK);
            _ksLedsResource.unlockResource(std::move(ksLeds));

            bool loopRunning = true;

            OPBOX_LOG_DEBUG("Attempting to initialize robot link for client %s", _settings.client.c_str());
            try
            {
                std::unique_ptr<RobotLink> robotLink = _robotLinkResource.lockResource();
                robotLink = std::make_unique<opbox::RobotLink>(
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

                _robotLinkResource.unlockResource(std::move(robotLink));
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
            std::unique_ptr<IOBuzzer> buzzer = _buzzerResource.lockResource();
            std::unique_ptr<IOUsrLed> usrLed = _usrLedResource.lockResource();
            
            //actuate IOs based on notification type
            switch(type)
            {
                case NOTIFICATION_WARNING:
                    //buzzer notification
                    buzzer->setState(IOBuzzerState::IO_BUZZER_CHIRP);
                    buzzer->setNextState(IOBuzzerState::IO_BUZZER_OFF, 500ms);

                    //user led
                    usrLed->setState(IOLedState::IO_LED_BLINK_ONCE);
                    usrLed->setNextState(IOLedState::IO_LED_OFF, 500ms);
                    break;
                case NOTIFICATION_ERROR:
                    //buzzer notification
                    buzzer->setState(IOBuzzerState::IO_BUZZER_LONG_CHIRP);
                    buzzer->setNextState(IOBuzzerState::IO_BUZZER_CHIRP_TWICE, 625ms);
                    buzzer->setNextState(IOBuzzerState::IO_BUZZER_OFF, 500ms);

                    //user led
                    usrLed->setState(IOLedState::IO_LED_BLINK_TWICE);
                    usrLed->setNextState(IOLedState::IO_LED_SLOW_BLINK, 500ms);
                    break;
                case NOTIFICATION_FATAL:
                    //buzzer notification
                    buzzer->setState(IOBuzzerState::IO_BUZZER_PANIC);
                    buzzer->setNextState(IOBuzzerState::IO_BUZZER_OFF, 4s);

                    //user led
                    usrLed->setState(IOLedState::IO_LED_FAST_BLINK);
                    break;
            }

            _buzzerResource.unlockResource(std::move(buzzer));
            _usrLedResource.unlockResource(std::move(usrLed));
        }


        void killButtonStateChanged(const int& newKillButtonState)
        {
            OPBOX_LOG_INFO("Kill button state changed to %d", newKillButtonState);
            ioActuatorAlert(NOTIFICATION_WARNING);
            
            std::unique_ptr<RobotLink> robotLink = _robotLinkResource.lockResource();
            if(robotLink)
            {
                robotLink->sendKillButtonState((KillSwitchState) newKillButtonState);
            }
            
            _robotLinkResource.unlockResource(std::move(robotLink));
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
            
            alert(NotificationType::NOTIFICATION_WARNING, "Robot connection", desc, "OK");
            ioActuatorAlert(NotificationType::NOTIFICATION_WARNING);

            if(connected && (!_browserProcess || !_browserProcess->running()))
            {
                std::string browserHost = _settings.customDiagServerIp;
                if(!_settings.useCustomDiagServerIp)
                {
                    browserHost = robot;
                }

                //start new browswer process
                std::string url = "http://" + browserHost + ":" + std::to_string(_settings.diagServerPort);
                std::vector<std::string> args = { url, }; // can add "kiosk" to make process fullscreen and not exitable
                OPBOX_LOG_DEBUG("Starting browser with url %s", url.c_str());

                _browserProcess = std::make_unique<Subprocess>(
                    "/usr/bin/sensible-browser", args);
                
                _browserProcess->run();
            }
        }


        void resetBuzzerAndUsrLed(void)
        {
            std::unique_ptr<IOBuzzer> buzzer = _buzzerResource.lockResource();
            std::unique_ptr<IOUsrLed> led = _usrLedResource.lockResource();

            buzzer->setState(IOBuzzerState::IO_BUZZER_OFF);
            led->setState(IOLedState::IO_LED_OFF);
            
            _buzzerResource.unlockResource(std::move(buzzer));
            _usrLedResource.unlockResource(std::move(led));
        }

        //loop
        std::mutex _loopRunningMutex;
        bool _loopRunning = true;

        //settings
        OpboxSettings _settings;

        //diagnostics web server
        std::unique_ptr<Subprocess> _browserProcess;

        //IO
        ProtectedResource<IOBuzzer> _buzzerResource;
        ProtectedResource<IOUsrLed> _usrLedResource;
        ProtectedResource<KillSwitchLeds> _ksLedsResource;
        ProtectedResource<IOGpioSensor<int>> _killSwitchResource;

        //communication
        ProtectedResource<RobotLink> _robotLinkResource;
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
