#include <libnotify/notify.h>
#include <iostream>

int main(int argc, char **argv)
{
    if(!notify_init("test_notification"))
    {
        std::cerr << "Failed to init libnotify" << std::endl;
    }

    NotifyNotification *notification = notify_notification_new(
        "Hello World!", 
        "this is a test notification.", 
        "dialog-information");

    if (!notify_notification_show(notification, NULL)) {
        std::cerr << "Failed to show notification" << std::endl;
        return EXIT_FAILURE;
    }

    g_object_unref(G_OBJECT(notification));
    notify_uninit();
}
