#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

namespace opbox
{
    OpboxRobotLink::OpboxRobotLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const KillButtonHandler& killButtonHandler,
        const StatusHandler& statusHandler)
     : handleNotification(notificationHandler),
       handleKillButton(killButtonHandler),
       handleStatus(statusHandler)
    {
        serial_library::SerialTransceiver::UniquePtr transceiver;
        if(address == "localhost")
        {
            transceiver = std::make_unique<serial_library::LinuxDualUDPTransceiver>(address, port, port + 1);
        } else 
        {
            transceiver = std::make_unique<serial_library::LinuxUDPTransceiver>(address, port);
        }
    }


    OpboxRobotLink::OpboxRobotLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const KillButtonHandler& killButtonHandler)
     : OpboxRobotLink(address, port, notificationHandler, killButtonHandler, std::bind(&OpboxRobotLink::defaultStatusHandler, this, _1, _2, _3, _4))
    { }


    OpboxRobotLink::OpboxRobotLink(
        const std::string& address,
        int port,
        const NotificationHandler& notificationHandler,
        const StatusHandler& statusHandler)
     : OpboxRobotLink(address, port, notificationHandler, std::bind(&OpboxRobotLink::defaultKillButtonHandler, this, _1), statusHandler)
    { }


    int OpboxRobotLink::nextNotificationUid = 0;
    int OpboxRobotLink::getNextNotificationUid()
    {
        return nextNotificationUid++;
    }


    bool OpboxRobotLink::connected()
    {
        if(!serialProc->hasDataForField(TIMESTAMP))
        {
            return false;
        }

        serial_library::SerialDataStamped data = serialProc->getField(TIMESTAMP);

        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                .count() - serial_library::convertFromCString<int64_t>(data.data.data, data.data.numData) < STALE_TIME;
    }


    bool OpboxRobotLink::sendNotificationToRemote(NotificationType type, const std::string& sensor, const std::string& desc)
    {
        
    }


    void OpboxRobotLink::sendKillButtonState(bool state)
    {
        serialProc->setField<int8_t>(KILL_BUTTON_STATE, (state ? 1 : 0), std::chrono::system_clock::now());
        auto now = std::chrono::system_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        //even if timestamp field is overwritten by an incoming message before send, itll still be close enough to the nearest time
        serialProc->setField<uint64_t>(TIMESTAMP, timestamp, now);
        serialProc->send(STATUS_OUT_FRAME);
    }


    void OpboxRobotLink::defaultNotificationHandler(NotificationType type, const std::string& sensorName, const std::string& desc)
    {
        OPBOX_LOG_DEBUG("Default notification handler reached; type: %s, sensor: %s, description: %s",
            notificationTypeToString(type).c_str(),
            sensorName.c_str(), desc.c_str());
    }


    void OpboxRobotLink::defaultStatusHandler(bool robotKillState, bool thrusterState, DiagnosticState diagState, bool leakState)
    {
        OPBOX_LOG_DEBUG("Default status handler reached; robot kill state: %d, thruster state: %d, diagnostic state: %s, leak state: %d",
            robotKillState, thrusterState,
            diagnosticStateToString(diagState),
            leakState);
    }


    void OpboxRobotLink::defaultKillButtonHandler(bool killButtonState)
    {
        OPBOX_LOG_DEBUG("Default kill button handler reached; kill button state: %d", killButtonState);
    }
}