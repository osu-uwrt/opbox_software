#include <libnotify/notify.h>
#include <iostream>

int main(int argc, char **argv)
{
    auto notification = notify_notification_new("Hello World!", "this is a test notification.", 0);
}
