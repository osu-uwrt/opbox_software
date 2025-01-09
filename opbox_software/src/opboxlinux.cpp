#include "opbox_software/opboxlinux.hpp"
#include "opbox_software/opboxlogging.hpp"
#include "opbox_software/opboxutil.hpp" //resolveAssetPath
#include <libnotify/notify.h>
#include <sys/wait.h>

using namespace std::placeholders;

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


    void alert(
        const NotificationType& type, 
        const std::string& header,
        const std::string& body,
        const std::string& button1Text,
        const std::string& button2Text,
        int timeoutSeconds,
        int defaultReturnCode,
        const ProcessReturnHandler& returnHandler)
    {
        std::thread t(&alertThread, type, header, body, button1Text, button2Text, timeoutSeconds, defaultReturnCode, returnHandler);
        t.detach();
    }


    void alertThread(
        const NotificationType& type, 
        const std::string& header,
        const std::string& body,
        const std::string& button1Text,
        const std::string& button2Text,
        int timeoutSeconds,
        int defaultReturnCode,
        const ProcessReturnHandler& returnHandler)
    {
        OPBOX_LOG_DEBUG("Creating alert with type %d, and header %s", type, header.c_str());
        Subprocess sp(
            resolveAssetPath("lib://opbox_alert"),
            {
                "--notification-type", notificationTypeToString(type),
                "--header", header,
                "--body", body,
                "--button1-text", button1Text,
                "--button2-enabled", (button2Text.empty() ? "false" : "true"),
                "--button2-text", button2Text,
                "--default-exit-code", std::to_string(defaultReturnCode),
                "--timeout", std::to_string(timeoutSeconds)
            },
            returnHandler
        );

        sp.run();
        sp.wait();
        OPBOX_LOG_DEBUG("Alert with type %d and header %s ended.", type, header.c_str());
    }


    Subprocess::Subprocess(const std::string& program, const std::vector<std::string>& args, const ProcessReturnHandler& returnHandler)
     : handleProcessReturn(returnHandler),
       program(program),
       args(args),
       retcode(0)
    { }


    Subprocess::Subprocess(
            const std::string& program,
            const std::vector<std::string>& args)
     : program(program),
       args(args),
       retcode(0)
    {
        handleProcessReturn = std::bind(&Subprocess::defaultProcessReturnHandler, this, _1);
    }


    void Subprocess::run()
    {
        thread = std::make_unique<std::thread>(
            std::bind(&Subprocess::threadFunc, this));
    }


    void Subprocess::wait()
    {
        thread->join();
    }


    bool Subprocess::running() const
    {
        return thread && thread->joinable();
    }


    void Subprocess::kill(const std::chrono::milliseconds& timeout)
    {
        OPBOX_LOG_DEBUG("Killing program %s", program.c_str());
        if(pid > 0)
        {
            for(int signal : {SIGINT, SIGTERM, SIGKILL})
            {
                OPBOX_LOG_DEBUG("Trying to kill program %s with signal %d", program.c_str(), signal);
                auto startTime = std::chrono::system_clock::now();
                while(::kill(pid, 0) == 0 && std::chrono::system_clock::now() - startTime < timeout)
                {
                    if(::kill(-pid, signal) < 0)
                    {
                        OPBOX_LOG_ERROR("Failed to kill program %s : %s", program.c_str(), strerror(errno));
                    }

                    std::this_thread::sleep_for(50ms);
                }

                if(::kill(pid, 0) != 0)
                {
                    //process still alive which is good
                    OPBOX_LOG_DEBUG("Program %s killed.", program.c_str());
                    break;
                }
            }
            
            if(::kill(pid, 0) == 0)
            {
                OPBOX_LOG_ERROR("Failed to kill program %s, it just wouldn't die.", program.c_str());
            }
        }
        
    }


    int Subprocess::returnCode() const
    {
        if(running())
        {
            OPBOX_LOG_ERROR("Subprocess return code requested but it is still running");
        }

        return retcode;
    }


    void Subprocess::threadFunc(void)
    {
        OPBOX_LOG_DEBUG("Forking and starting program %s", program.c_str());

        pid = fork();
        if(pid == -1)
        {
            OPBOX_LOG_ERROR("Failed to fork(): %s", strerror(errno));
        }

        if(pid == 0)
        {
            //child process, assemble args into c style string array and start program
            char *execArgs[(const size_t) args.size() + 2];
            execArgs[0] = (char*) malloc(program.length());
            strcpy(execArgs[0], program.c_str());
            for(int i = 0; i < args.size(); i++)
            {
                execArgs[i + 1] = (char*) malloc(args[i].length());
                strcpy(execArgs[i + 1], args[i].c_str());
            }

            execArgs[args.size() + 1] = (char*) 0;

            char *useruid = getenv("SUDO_UID");
            if(useruid)
            {
                OPBOX_LOG_DEBUG("Setting uid of process %s to %s", program.c_str(), useruid);
                uid_t uid = (uid_t) atoi(useruid);

                if(uid && setuid(uid) != 0)
                {
                    OPBOX_LOG_ERROR("Unable to set uid of program %s: %s", program.c_str(), strerror(errno));
                }
            }

            OPBOX_LOG_DEBUG("Starting program %s via execv() with args listed below:", program.c_str());
            for(size_t i = 0; i < args.size() + 2; i++)
            {
                OPBOX_LOG_DEBUG("Program %s, arg %d: %s", program.c_str(), i, execArgs[i]);
            }

            int res = execv(program.c_str(), execArgs);
            if(res == -1)
            {
                OPBOX_LOG_ERROR("execve() failed for program %s: %s", program.c_str(), strerror(errno));
                exit(-1);
            }

            //free c style args
            for(int i = 0; i < args.size(); i++)
            {
                free(execArgs[i]);
            }
        } else
        {
            //parent process
            int status = 0;
            bool exited = false;

            OPBOX_LOG_DEBUG("Successfully started process as pid %d", pid);

            while(!exited)
            {
                if(waitpid(pid, &status, 0) < 1) //wait for program to stop
                {
                    OPBOX_LOG_ERROR("waitpid() failed: %s", strerror(errno));
                    break;
                }

                exited = WIFSIGNALED(status) || WIFEXITED(status);
                OPBOX_LOG_DEBUG("Program %s state changed, status %d, exited: %s", program.c_str(), status, exited ? "yes" : "no");
            }

            retcode = 0;
            if(WIFEXITED(status))
            {
                retcode = WEXITSTATUS(status);
            }

            OPBOX_LOG_DEBUG("Program %s exited with code %d, running return function", program.c_str(), retcode);
            handleProcessReturn(retcode);

            // //handle return
            // if()
            // {
                
            // } else
            // {
            //     //uh oh
            //     OPBOX_LOG_ERROR("waitpid() returned but program %s has not exited", program.c_str());
            // }
        }
    }


    void Subprocess::defaultProcessReturnHandler(int retcode)
    {
        OPBOX_LOG_DEBUG("Program %s returned with code %d", program.c_str(), retcode);
    }
}
