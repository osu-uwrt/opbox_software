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
            diagnosticStateToString(diagState).c_str(),
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
        const StatusHandler& statusHandler,
        const ConnectionStateHandler& conStateHandler,
        const OpboxFrameId& bumpFrameId,
        const std::string& debugName)
     : handleNotification(notificationHandler),
       handleKillButton(killButtonHandler),
       handleStatus(statusHandler),
       handleConnectionStateChange(conStateHandler),
       threadRunning(true),
       bumpFrameId(bumpFrameId),
       debugName(debugName)
    {
        OPBOX_LOG_DEBUG("%s constructing serial processor", debugName.c_str());
        serialProc = std::make_unique<serial_library::SerialProcessor>(
            std::move(transceiver),
            OPBOX_FRAMES,
            NOTHING_FRAME,
            OPBOX_SYNC,
            sizeof(OPBOX_SYNC),
            getCallbacks(),
            debugName);

        OPBOX_LOG_DEBUG("%s serial processor constructed, continuing initialization", debugName.c_str());
        
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

        if(thread->joinable())
        {
            thread->join();
        }
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
        throw std::runtime_error(debugName + ": sendKillButtonState cannot be used in this context.");
    }


    void OpboxRobotLink::sendRobotState(const KillSwitchState& killState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState)
    {
        throw std::runtime_error(debugName + ": sendRobotState cannot be used in this context.");
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
            case ROBOT_STATUS_FRAME:
                OPBOX_LOG_DEBUG("%s: New robot status received", debugName.c_str());

                handleStatus(
                    (KillSwitchState) serialProc->getFieldValue<uint8_t>(ROBOT_KILL_STATE),
                    (ThrusterState) serialProc->getFieldValue<uint8_t>(THRUSTER_STATE),
                    (DiagnosticState) serialProc->getFieldValue<uint8_t>(DIAGNOSTICS_STATE),
                    (LeakState) serialProc->getFieldValue<uint8_t>(LEAK_STATE)
                );

                break;
            
            case OPBOX_STATUS_FRAME:
                OPBOX_LOG_DEBUG("%s: New opbox status received", debugName.c_str());

                handleKillButton(
                    (KillSwitchState) serialProc->getFieldValue<uint8_t>(KILL_BUTTON_STATE)
                );

                break;
            case NOTIFICATION_FRAME:
                {
                    //handle notification with callback
                    NotificationUid uid = (NotificationUid) serialProc->getFieldValue<uint8_t>(NOTIFICATION_UID);
                    if(handledUids.count(uid) == 0)
                    {
                        handleNotification(
                            (NotificationType) serialProc->getFieldValue<uint8_t>(NOTIFICATION_TYPE),
                            serialProc->getField(NOTIFICATION_SENSOR_NAME).data.data,
                            serialProc->getField(NOTIFICATION_DESCRIPTION).data.data);
                        
                        handledUids.insert(uid);
                    }
                    
                    //send acknowledgement of notification
                    serialProc->setFieldValue<uint8_t>(ACKED_NOTIFICATION_UID, uid, std::chrono::system_clock::now());
                    serialProc->send(ACK_FRAME);
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
        serialProc->setFieldValue<uint8_t>(NOTIFICATION_UID, uid, now);
        serialProc->setFieldValue<uint8_t>(NOTIFICATION_TYPE, type, now);
        serialProc->setField(NOTIFICATION_SENSOR_NAME, serial_library::serialDataFromString(sensor), now);
        serialProc->setField(NOTIFICATION_DESCRIPTION, serial_library::serialDataFromString(desc), now);
        serialProc->send(NOTIFICATION_FRAME);
    }


    bool OpboxRobotLink::waitForAck(const NotificationUid& uid, const std::chrono::milliseconds& timeout)
    {
        auto startTime = std::chrono::system_clock::now();
        std::this_thread::sleep_for(1ms);
        bool acked = (NotificationUid) serialProc->getFieldValue<uint8_t>(ACKED_NOTIFICATION_UID) == uid;
        
        while(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now() - startTime) < timeout && !acked)
        {
            acked = (NotificationUid) serialProc->getFieldValue<uint8_t>(ACKED_NOTIFICATION_UID) == uid;
        }

        return acked;
    }


    void OpboxRobotLink::threadFunc(void)
    {
        OPBOX_LOG_DEBUG("%s: OpboxRobotLink thread starting", debugName.c_str());
        bool connectionState = false;
        while(threadRunning)
        {
            auto now = std::chrono::system_clock::now();
            serialProc->update(now);

            if(now - lastSendTime > 100ms)
            {
                serialProc->send(bumpFrameId);
                lastSendTime = now;
            }

            KillSwitchState a = (KillSwitchState) serialProc->getFieldValue<uint8_t>(KILL_BUTTON_STATE);
            OPBOX_LOG_DEBUG("%s: kill button state is currently %d", debugName.c_str(), a);

            bool newConnectionState = connected();
            if(connectionState != newConnectionState)
            {
                handleConnectionStateChange(newConnectionState);
            }

            connectionState = newConnectionState;

            //this gives other threads chance to aquire mutexes for sending data as std::mutex is not necessarily fair
            std::this_thread::sleep_for(5ms); 
        }

        OPBOX_LOG_DEBUG("%s: OpboxRobotLink thread ending", debugName.c_str());
    }


    RobotLink::RobotLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const StatusHandler& statusHandler,
        const ConnectionStateHandler& connectionStateHandler)
     : OpboxRobotLink(
        std::move(buildTransceiver(address, port)),
        notificationHandler,
        &defaultKillButtonHandler,
        statusHandler,
        connectionStateHandler,
        OPBOX_STATUS_FRAME,
        "RobotLink")
    { }
    

    void RobotLink::sendKillButtonState(const KillSwitchState& state)
    {
        OPBOX_LOG_DEBUG("Sending opbox state with kill button state %d", state);
        auto now = std::chrono::system_clock::now();
        serialProc->setFieldValue<uint8_t>(KILL_BUTTON_STATE, state, now);
        serialProc->send(OPBOX_STATUS_FRAME);
        lastSendTime = now;
    }


    serial_library::SerialTransceiver::UniquePtr RobotLink::buildTransceiver(const std::string& address, int port)
    {
        if(address == "localhost")
        {
            OPBOX_LOG_DEBUG("RobotLink using LinuxDualUDPTransceiver for localhost");
            return std::make_unique<serial_library::LinuxDualUDPTransceiver>(address, port, port + 1, 0.1, true);
        }

        return std::make_unique<serial_library::LinuxUDPTransceiver>(address, port, 0.1);
    }


    OpboxLink::OpboxLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const KillButtonHandler& killButtonHandler,
        const ConnectionStateHandler& connectionStateHandler)
     : OpboxRobotLink(
        buildTransceiver(address, port),
        notificationHandler,
        killButtonHandler,
        &defaultStatusHandler,
        connectionStateHandler,
        ROBOT_STATUS_FRAME,
        "OpboxLink")
    { }


    void OpboxLink::sendRobotState(const KillSwitchState& killState, const ThrusterState& thrusterState, const DiagnosticState& diagState, const LeakState& leakState)
    {
        OPBOX_LOG_DEBUG("Sending robot state");
        auto now = std::chrono::system_clock::now();
        serialProc->setFieldValue<uint8_t>(ROBOT_KILL_STATE, killState, now);
        serialProc->setFieldValue<uint8_t>(THRUSTER_STATE, thrusterState, now);
        serialProc->setFieldValue<uint8_t>(DIAGNOSTICS_STATE, diagState, now);
        serialProc->setFieldValue<uint8_t>(LEAK_STATE, leakState, now);
        serialProc->send(ROBOT_STATUS_FRAME);
        lastSendTime = now;
    }


    serial_library::SerialTransceiver::UniquePtr OpboxLink::buildTransceiver(const std::string& address, int port)
    {
        if(address == "localhost")
        {
            OPBOX_LOG_DEBUG("OpboxLink using LinuxDualUDPTransceiver for localhost");
            return std::make_unique<serial_library::LinuxDualUDPTransceiver>(address, port + 1, port, 0.1, true);
        }

        return std::make_unique<serial_library::LinuxUDPTransceiver>(address, port, 0.1);
    }
}