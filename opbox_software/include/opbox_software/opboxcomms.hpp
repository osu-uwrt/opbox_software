#pragma once

#include "opbox_software/opboxbase.hpp"
#include "opbox_software/opboxframes.hpp"
#include "opbox_software/opboxlogging.hpp"
#include <set>
#include <thread>

#define HEARTBEAT_RATE 100ms

// this is in ms
#define STALE_TIME 500ms

using namespace std::chrono_literals;

namespace opbox
{
    static void defaultNotificationHandler(const NotificationType& type, const std::string& sensorName, const std::string& desc);
    static void defaultStatusHandler(const KillSwitchState& robotKillState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState);
    static void defaultKillButtonHandler(const KillSwitchState& killButtonState);

    class OpboxRobotLink
    {
        public:
        typedef std::unique_ptr<OpboxRobotLink> UniquePtr;
        typedef std::function<void(const NotificationType&, const std::string&, const std::string&)> NotificationHandler;
        typedef std::function<void(const KillSwitchState&, const ThrusterState&, const DiagnosticState&, const LeakState&)> StatusHandler;
        typedef std::function<void(const KillSwitchState&)> KillButtonHandler;

        OpboxRobotLink(
            std::unique_ptr<serial_library::SerialTransceiver> transceiver,
            const NotificationHandler& notificationHandler,
            const KillButtonHandler& killButtonHandler,
            const StatusHandler& statusHandler,
            const OpboxFrameId& bumpFrameId,
            const std::string& debugName);

        ~OpboxRobotLink();

        static NotificationUid getNextNotificationUid(void);
        bool connected(void);
        bool sendNotification(
            const NotificationType& type,
            const std::string& sensor,
            const std::string& desc,
            const std::chrono::milliseconds& timeout = 500ms);
        
        //these are default functions which throw and should be overridden.
        virtual void sendKillButtonState(const KillSwitchState& state);
        virtual void sendRobotState(const KillSwitchState& killState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState);

        protected:
        serial_library::SerialProcessor::UniquePtr serialProc;
        const OpboxFrameId bumpFrameId;
        std::chrono::time_point<std::chrono::system_clock> lastSendTime;

        private:
        serial_library::SerialProcessorCallbacks getCallbacks(void);
        void newMessageCallback(const serial_library::SerialValuesMap& values);
        void trySendNotificationToRemote(
            const NotificationUid& uid,
            const NotificationType& type,
            const std::string& sensor,
            const std::string& desc);
        bool waitForAck(const NotificationUid& uid, const std::chrono::milliseconds& timeout);
        void threadFunc(void);

        static NotificationUid nextNotificationUid;
        std::set<NotificationUid> handledUids;

        const NotificationHandler handleNotification;
        const KillButtonHandler handleKillButton;
        const StatusHandler handleStatus;
        const std::string debugName;

        //thread
        bool threadRunning;
        std::unique_ptr<std::thread> thread;
    };


    //link to be used by opbox side to communicate with robot
    class RobotLink : public OpboxRobotLink
    {
        public:
        RobotLink(
            const std::string& address,
            int port,
            const NotificationHandler& notificationHandler,
            const StatusHandler& statusHandler);
        
        void sendKillButtonState(const KillSwitchState& state) override;

        private:
        static serial_library::SerialTransceiver::UniquePtr buildTransceiver(const std::string& address, int port);
    };


    //link to be used by opbox side to communicate with robot
    class OpboxLink : public OpboxRobotLink
    {
        public:
        OpboxLink(
            const std::string& address,
            int port,
            const NotificationHandler& notificationHandler,
            const KillButtonHandler& killButtonHandler);
        
        void sendRobotState(const KillSwitchState& killState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState) override;

        private:
        static serial_library::SerialTransceiver::UniquePtr buildTransceiver(const std::string& address, int port);
    };
}
