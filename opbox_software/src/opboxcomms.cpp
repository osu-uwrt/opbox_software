#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

namespace opbox
{
    void defaultNotificationHandler(const NotificationType& type, const std::string& sensorName, const std::string& desc)
    {
        OPBOX_LOG_DEBUG("Default notification handler reached; type: %s, sensor: %s, description: %s",
            notificationTypeToString(type).c_str(),
            sensorName.c_str(), desc.c_str());
    }


    void defaultStatusHandler(const KillSwitchState& robotKillState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState)
    {
        OPBOX_LOG_DEBUG("Default status handler reached; robot kill state: %d, thruster state: %d, diagnostic state: %s, leak state: %d",
            robotKillState, thrusterState,
            diagnosticStateToString(diagState),
            leakState);
    }


    void defaultKillButtonHandler(const KillSwitchState& killButtonState)
    {
        OPBOX_LOG_DEBUG("Default kill button handler reached; kill button state: %d", killButtonState);
    }


    OpboxRobotLink::OpboxRobotLink(
        std::unique_ptr<serial_library::SerialTransceiver> transceiver,
        const NotificationHandler& notificationHandler,
        const KillButtonHandler& killButtonHandler,
        const StatusHandler& statusHandler)
     : handleNotification(notificationHandler),
       handleKillButton(killButtonHandler),
       handleStatus(statusHandler),
       threadRunning(true)
    {
        serialProc = std::make_unique<serial_library::SerialProcessor>(
            std::move(transceiver),
            OPBOX_FRAMES,
            STATUS_FRAME,
            OPBOX_SYNC,
            sizeof(OPBOX_SYNC),
            getCallbacks());
        
        auto now = std::chrono::system_clock::now();

        serialProc->setFieldValue<KillSwitchState>(ROBOT_KILL_STATE, KillSwitchState::KILLED, now);
        serialProc->setFieldValue<ThrusterState>(THRUSTER_STATE, ThrusterState::IDLE, now);
        serialProc->setFieldValue<DiagnosticState>(DIAGNOSTICS_STATE, DiagnosticState::DIAGNOSTICS_OK, now);
        serialProc->setFieldValue<LeakState>(LEAK_STATE, LeakState::OK, now);
        serialProc->setFieldValue<KillSwitchState>(KILL_BUTTON_STATE, KillSwitchState::KILLED, now);

        thread = std::make_unique<std::thread>(std::bind(&OpboxRobotLink::threadFunc, this));
    }


    OpboxRobotLink::~OpboxRobotLink()
    {
        threadRunning = false;
        thread->join();
    }


    NotificationUid OpboxRobotLink::nextNotificationUid = 0;
    NotificationUid OpboxRobotLink::getNextNotificationUid()
    {
        return nextNotificationUid++;
    }


    bool OpboxRobotLink::connected()
    {
        auto elapsedSinceLastmsg = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - serialProc->getLastMsgRecvTime());
        
        return elapsedSinceLastmsg < STALE_TIME;
    }


    bool OpboxRobotLink::sendNotification(
        const NotificationType& type,
        const std::string& sensor,
        const std::string& desc,
        const std::chrono::milliseconds& timeout)
    {
        auto startTime = std::chrono::system_clock::now();

        NotificationUid uid = getNextNotificationUid();
        bool acked = false;
        while(!acked && 
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startTime) < timeout)
        {
            trySendNotificationToRemote(uid, type, sensor, desc);
            acked = waitForAck(uid, 50ms);
        }

        return acked;
    }


    void OpboxRobotLink::sendKillButtonState(const KillSwitchState& state)
    {
        throw std::runtime_error("sendKillButtonState cannot be used in this context.");
    }


    void OpboxRobotLink::sendRobotState(const KillSwitchState& killState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState)
    {
        throw std::runtime_error("sendRobotState cannot be used in this context.");
    }


    serial_library::SerialProcessorCallbacks OpboxRobotLink::getCallbacks(void)
    {
        serial_library::SerialProcessorCallbacks callbacks;
        callbacks.newMessageCallback = std::bind(&OpboxRobotLink::newMessageCallback, this, _1);
        return callbacks;
    }


    void OpboxRobotLink::newMessageCallback(const serial_library::SerialValuesMap& values)
    {
        OpboxFrameId frameType = serial_library::convertData<OpboxFrameId>(values.at(FIELD_FRAME));
        
        switch(frameType)
        {
            case STATUS_FRAME:
                handleStatus(
                    serialProc->getFieldValue<KillSwitchState>(ROBOT_KILL_STATE),
                    serialProc->getFieldValue<ThrusterState>(THRUSTER_STATE),
                    serialProc->getFieldValue<DiagnosticState>(DIAGNOSTICS_STATE),
                    serialProc->getFieldValue<LeakState>(LEAK_STATE)
                );

                handleKillButton(
                    serialProc->getFieldValue<KillSwitchState>(KILL_BUTTON_STATE)
                );

                break;
            case NOTIFICATION_FRAME:
                {
                    //handle notification with callback
                    NotificationUid uid = serialProc->getFieldValue<NotificationUid>(NOTIFICATION_UID);
                    if(handledUids.count(uid) == 0)
                    {
                        handleNotification(
                            serialProc->getFieldValue<NotificationType>(NOTIFICATION_TYPE),
                            serialProc->getField(NOTIFICATION_SENSOR_NAME).data.data,
                            serialProc->getField(NOTIFICATION_DESCRIPTION).data.data);
                        
                        handledUids.insert(uid);
                    }
                    
                    //send acknowledgement of notification
                    serialProc->setFieldValue<NotificationUid>(ACKED_NOTIFICATION_UID, uid, std::chrono::system_clock::now());
                }
                break;
            default:
                break;
        }

    }


    void OpboxRobotLink::trySendNotificationToRemote(
        const NotificationUid& uid,
        const NotificationType& type,
        const std::string& sensor,
        const std::string& desc)
    {
        auto now = std::chrono::system_clock::now();
        serialProc->setFieldValue<NotificationUid>(NOTIFICATION_UID, uid, now);
        serialProc->setFieldValue<NotificationType>(NOTIFICATION_TYPE, type, now);
        serialProc->setField(NOTIFICATION_SENSOR_NAME, serial_library::serialDataFromString(sensor), now);
        serialProc->setField(NOTIFICATION_DESCRIPTION, serial_library::serialDataFromString(desc), now);
        serialProc->send(NOTIFICATION_FRAME);
    }


    bool OpboxRobotLink::waitForAck(const NotificationUid& uid, const std::chrono::milliseconds& timeout)
    {
        auto startTime = std::chrono::system_clock::now();
        std::this_thread::sleep_for(1ms);
        bool acked = serialProc->getFieldValue<NotificationUid>(ACKED_NOTIFICATION_UID) == uid;
        
        while(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - startTime) < timeout && !acked)
        {
            acked = serialProc->getFieldValue<NotificationUid>(ACKED_NOTIFICATION_UID) == uid;
        }

        return acked;
    }


    void OpboxRobotLink::threadFunc(void)
    {
        OPBOX_LOG_DEBUG("OpboxRobotLink thread starting");
        while(threadRunning)
        {
            auto now = std::chrono::system_clock::now();
            serialProc->update(now);
        }

        OPBOX_LOG_DEBUG("OpboxRobotLink thread ending");
    }


    RobotLink::RobotLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const StatusHandler& statusHandler)
     : OpboxRobotLink(
        std::move(buildTransceiver(address, port)),
        notificationHandler,
        &defaultKillButtonHandler,
        statusHandler)
    { }
    

    void RobotLink::sendKillButtonState(const KillSwitchState& state)
    {
        auto now = std::chrono::system_clock::now();
        serialProc->setFieldValue<KillSwitchState>(KILL_BUTTON_STATE, state, now);
        serialProc->send(STATUS_FRAME);
    }


    serial_library::SerialTransceiver::UniquePtr RobotLink::buildTransceiver(const std::string& address, int port)
    {
        if(address == "localhost")
        {
            OPBOX_LOG_DEBUG("RobotLink using LinuxDualUDPTransceiver for localhost");
            return std::make_unique<serial_library::LinuxDualUDPTransceiver>(address, port, port + 1);
        }

        return std::make_unique<serial_library::LinuxUDPTransceiver>(address, port);
    }


    OpboxLink::OpboxLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const KillButtonHandler& killButtonHandler)
     : OpboxRobotLink(
        buildTransceiver(address, port),
        notificationHandler,
        killButtonHandler,
        &defaultStatusHandler)
    { }


    void OpboxLink::sendRobotState(const KillSwitchState& killState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState)
    {
        auto now = std::chrono::system_clock::now();
        serialProc->setFieldValue<KillSwitchState>(ROBOT_KILL_STATE, killState, now);
        serialProc->setFieldValue<ThrusterState>(THRUSTER_STATE, thrusterState, now);
        serialProc->setFieldValue<DiagnosticState>(DIAGNOSTICS_STATE, diagState, now);
        serialProc->setFieldValue<LeakState>(LEAK_STATE, leakState, now);
        serialProc->send(STATUS_FRAME);
    }


    serial_library::SerialTransceiver::UniquePtr OpboxLink::buildTransceiver(const std::string& address, int port)
    {
        if(address == "localhost")
        {
            OPBOX_LOG_DEBUG("OpboxLink using LinuxDualUDPTransceiver for localhost");
            return std::make_unique<serial_library::LinuxDualUDPTransceiver>(address, port + 1, port);
        }

        return std::make_unique<serial_library::LinuxUDPTransceiver>(address, port);
    }
}