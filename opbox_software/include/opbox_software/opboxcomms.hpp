#pragma once

#include "opbox_software/opboxbase.hpp"
#include "opbox_software/opboxframes.hpp"
#include "opbox_software/opboxlogging.hpp"

#define HEARTBEAT_RATE 100ms

// this is in ms
#define STALE_TIME 500

namespace opbox
{
    class OpboxRobotLink
    {
        public:
        typedef std::unique_ptr<OpboxRobotLink> UniquePtr;
        typedef std::function<void(NotificationType, const std::string&, const std::string&)> NotificationHandler;
        typedef std::function<void(bool, bool, DiagnosticState, bool)> StatusHandler;
        typedef std::function<void(bool)> KillButtonHandler;

        OpboxRobotLink(
            const std::string& address,
            int port,
            const NotificationHandler& notificationHandler,
            const KillButtonHandler& killButtonHandler,
            const StatusHandler& statusHandler);

        OpboxRobotLink(
            const std::string& address,
            int port,
            const NotificationHandler& notificationHandler,
            const KillButtonHandler& killButtonHandler);
        
        OpboxRobotLink(
            const std::string& address,
            int port,
            const NotificationHandler& notificationHandler,
            const StatusHandler& statusHandler);

        static int getNextNotificationUid(void);
        bool connected(void);
        bool sendNotificationToRemote(NotificationType type, const std::string& sensor, const std::string& desc);
        void sendKillButtonState(bool state);

        private:
        void defaultNotificationHandler(NotificationType type, const std::string& sensorName, const std::string& desc);
        void defaultStatusHandler(bool robotKillState, bool thrusterState, DiagnosticState diagState, bool leakState);
        void defaultKillButtonHandler(bool killButtonState);

        static int nextNotificationUid;
        serial_library::SerialProcessor::UniquePtr serialProc;

        const NotificationHandler handleNotification;
        const KillButtonHandler handleKillButton;
        const StatusHandler handleStatus;
    };
}
