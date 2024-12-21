#pragma once

#include <serial_library/serial_library.hpp>

namespace opbox
{
    enum OpboxFrameId
    {
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
        NOTIFICATION_DESCRIPTION
    };

    enum DiagnosticState
    {
        DIAGNOSTICS_OK,
        DIAGNOSTICS_WARN,
        DIAGNOSTICS_ERROR
    };

    enum NotificationType
    {
        NOTIFICATION_WARNING,
        NOTIFICATION_ERROR,
        NOTIFICATION_FATAL
    };

    const SerialFramesMap OPBOX_FRAMES = {
        {
            STATUS_IN_FRAME, //will be received by client running on robot
            {
                FIELD_SYNC,
                FIELD_FRAME,
                ROBOT_KILL_STATE,
                DIAGNOSTICS_STATE,
                LEAK_STATE,
                TIMESTAMP,
                TIMESTAMP,
                TIMESTAMP,
            }
        },
        {
            STATUS_OUT_FRAME, //will be sent to client running on robot
            {
                FIELD_SYNC,
                FIELD_FRAME,
                KILL_BUTTON_STATE,
                TIMESTAMP,
                TIMESTAMP,
                TIMESTAMP,
            }
        },
        {
            NOTIFICATION_FRAME,
            {
                FIELD_SYNC,
                FIELD_FRAME,
                NOTIFICATION_TYPE,
                NOTIFICATION_UID,
                NOTIFICATION_SENSOR_NAME,
                NOTIFICATION_DESCRIPTION,
                TIMESTAMP,
                TIMESTAMP,
                TIMESTAMP
            }
        },
        {
            ACK_FRAME,
            {
                FIELD_SYNC,
                FIELD_FRAME,
                NOTIFICATION_TYPE,
                NOTIFICATION_UID,
                TIMESTAMP,
                TIMESTAMP,
                TIMESTAMP
            }
        }
    };
}
