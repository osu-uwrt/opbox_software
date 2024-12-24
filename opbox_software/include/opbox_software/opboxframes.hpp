#pragma once

#include <serial_library/serial_library.hpp>

namespace opbox
{
    enum OpboxFrameId
    {
        HEARTBEAT_FRAME,
        STATUS_IN_FRAME,
        STATUS_OUT_FRAME,
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
        TIMESTAMP,
        NOTIFICATION_SENSOR_NAME,
        NOTIFICATION_TYPE,
        NOTIFICATION_UID,
        NOTIFICATION_DESCRIPTION,

    };

    enum DiagnosticState
    {
        DIAGNOSTICS_OK,
        DIAGNOSTICS_WARN,
        DIAGNOSTICS_ERROR
    };

    std::string diagnosticStateToString(const DiagnosticState& state)
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

    std::string notificationTypeToString(const NotificationType& type)
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

    const serial_library::SerialFramesMap OPBOX_FRAMES = {
        {
            HEARTBEAT_FRAME, //this frame keeps the timestamp up to date 
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, 1 },
                { FIELD_FRAME, 1 },
                { TIMESTAMP, 8 }
            })
        },
        {
            STATUS_IN_FRAME, //will be received by client running on robot
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, 1 },
                { FIELD_FRAME, 1 },
                { ROBOT_KILL_STATE, 1 },
                { DIAGNOSTICS_STATE, 1 },
                { LEAK_STATE, 1 },
                { TIMESTAMP, 8 }
            })
        },
        {
            STATUS_OUT_FRAME, //will be sent to client running on robot
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, 1 },
                { FIELD_FRAME, 1 },
                { KILL_BUTTON_STATE, 1 },
                { TIMESTAMP, 8 }
            })
        },
        {
            NOTIFICATION_FRAME,
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, 1 },
                { FIELD_FRAME, 1 },
                { NOTIFICATION_TYPE, 1 },
                { NOTIFICATION_UID, 1 },
                { NOTIFICATION_SENSOR_NAME, 16 },
                { NOTIFICATION_DESCRIPTION, 64 },
                { TIMESTAMP, 8 }
            })
        },
        {
            ACK_FRAME,
            serial_library::assembleSerialFrame({
                { FIELD_SYNC, 1 },
                { FIELD_FRAME, 1 },
                { NOTIFICATION_TYPE, 1 },
                { NOTIFICATION_UID, 1 },
                { TIMESTAMP, 8 }
            })
        }
    };
}
