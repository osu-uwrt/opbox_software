#pragma once

#include "opbox_software/opboxcomms.hpp"
#include "opbox_software/opboxbase.hpp"
#include <vector>
#include <thread>

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
        typedef std::function<void(int)> ProcessReturnHandler;

        Subprocess(
            const std::string& program,
            const std::vector<std::string>& args,
            const ProcessReturnHandler& returnHandler);
        
        Subprocess(
            const std::string& program,
            const std::vector<std::string>& args = {});
        
        void run();
        void wait();
        bool running() const;
        void kill();
        int returnCode() const;

        private:
        void threadFunc(void);
        void defaultProcessReturnHandler(int retcode);

        const std::string program;
        const std::vector<std::string> args;
        ProcessReturnHandler handleProcessReturn;
        std::unique_ptr<std::thread> thread;
        int retcode;
        pid_t pid;
    };
}
