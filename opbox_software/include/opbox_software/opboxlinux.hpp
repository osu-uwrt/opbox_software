#pragma once

#include "opbox_software/opboxcomms.hpp"
#include "opbox_software/opboxbase.hpp"
#include <vector>

//
// Opbox header for Linux system operations
//

namespace opbox
{
    bool initializeNotifications(void);
    void sendSystemNotification(const NotificationType& notificationType, const std::string& title, const std::string& desc);
    void deinitializeNotifications(void);

    class Subprocess
    {
        public:
        Subprocess(const std::string& program, const std::vector<std::string>& args);
        void run();
        bool running();
        void kill();

        private:
        const std::string program;
        const std::vector<std::string> args;
        pid_t pid;
    };
}
