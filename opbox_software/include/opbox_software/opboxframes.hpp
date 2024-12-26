#pragma once

#include <serial_library/serial_library.hpp>

namespace opbox
{
    typedef uint16_t NotificationUid;

    enum OpboxFrameId
    {
        STATUS_FRAME,
        NOTIFICATION_FRAME,
        ACK_FRAME
    };

    enum OpboxField
    {
        ROBOT_KILL_STATE,
        THRUSTER_STATE,
        DIAGNOSTICS_STATE,
        LEAK_STATE,
        KILL_BUTTON_STATE,
        NOTIFICATION_SENSOR_NAME,
        NOTIFICATION_TYPE,
        NOTIFICATION_UID,
        ACKED_NOTIFICATION_UID,
        NOTIFICATION_DESCRIPTION
    };

    enum KillSwitchState
    {
        UNKILLED,
        KILLED
    };

    enum ThrusterState
    {
        IDLE,
        ACTIVE
    };

    enum LeakState
    {
        OK,
        LEAKING
    };

    enum DiagnosticState
    {
        DIAGNOSTICS_OK,
        DIAGNOSTICS_WARN,
        DIAGNOSTICS_ERROR
    };

    static std::string diagnosticStateToString(const DiagnosticState& state)
    {
        switch(state)
        {
            case DIAGNOSTICS_OK:
                return "DIAGNOSTICS_OK";
            case DIAGNOSTICS_WARN:
                return "DIAGNOSTICS_WARN";
            case DIAGNOSTICS_ERROR:
                return "DIAGNOSTICS_ERROR";
            default:
                return "BAD STATE";
        }
    }

    enum NotificationType
    {
        NOTIFICATION_WARNING,
        NOTIFICATION_ERROR,
        NOTIFICATION_FATAL
    };

    static std::string notificationTypeToString(const NotificationType& type)
    {
        switch(type)
        {
            case NOTIFICATION_WARNING:
                return "NOTIFICATION_WARNING";
            case NOTIFICATION_ERROR:
                return "NOTIFICATION_ERROR";
            case NOTIFICATION_FATAL:
                return "NOTIFICATION_FATAL";
            default:
                return "BAD TYPE";
        }
    }

    const char OPBOX_SYNC[] = "*";

    const serial_library::SerialFramesMap OPBOX_FRAMES = {
        {
            STATUS_FRAME, //will be received by client running on robot
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, sizeof(OPBOX_SYNC) },
                { FIELD_FRAME, 1 },
                { ROBOT_KILL_STATE, 1 },
                { KILL_BUTTON_STATE, 1 },
                { THRUSTER_STATE, 1 },
                { DIAGNOSTICS_STATE, 1 },
                { LEAK_STATE, 1 },
                { FIELD_CHECKSUM, 2 }
            })
        },
        {
            NOTIFICATION_FRAME,
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, sizeof(OPBOX_SYNC) },
                { FIELD_FRAME, 1 },
                { NOTIFICATION_TYPE, 1 },
                { NOTIFICATION_UID, 1 },
                { NOTIFICATION_SENSOR_NAME, 16 },
                { NOTIFICATION_DESCRIPTION, 64 },
                { FIELD_CHECKSUM, 2 }
            })
        },
        {
            ACK_FRAME,
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, sizeof(OPBOX_SYNC) },
                { FIELD_FRAME, 1 },
                { ACKED_NOTIFICATION_UID, 1 },
                { FIELD_CHECKSUM, 2 }
            })
        }
    };
}
