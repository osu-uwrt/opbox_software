#include "opbox_software/opboxlinux.hpp"
#include "opbox_software/opboxlogging.hpp"
#include <libnotify/notify.h>

namespace opbox
{
    bool libNotifyIsInitted = false;
    bool initializeNotifications(void)
    {
        if(!libNotifyIsInitted)
        {
            //try to init libnotify
            libNotifyIsInitted = notify_init("test_notification");
            if(!libNotifyIsInitted)
            {
                return false;
            }
        }

        return true;
    }


    void sendSystemNotification(const NotificationType& notificationType, const std::string& title, const std::string& desc)
    {
        if(!libNotifyIsInitted)
        {
            OPBOX_LOG_ERROR("Cannot create system notification because libnotify is not initialized.");
            return;
        }

        std::string icon = "dialog-information";
        switch(notificationType)
        {
            case NOTIFICATION_WARNING:
                icon = "dialog-information";
                break;
            case NOTIFICATION_ERROR:
                icon = "dialog-warning";
                break;
            case NOTIFICATION_FATAL:
                icon = "dialog-error";
                break;
        }

        NotifyNotification *notification = notify_notification_new(
            title.c_str(), 
            desc.c_str(), 
            icon.c_str());

        if (!notify_notification_show(notification, NULL)) {
            OPBOX_LOG_ERROR("Failed to show notification");
        }

        g_object_unref(G_OBJECT(notification));
    }


    void deinitializeNotifications(void)
    {
        notify_uninit();
        libNotifyIsInitted = false;
    }
}
