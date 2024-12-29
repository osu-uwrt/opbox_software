#include "opbox_software/opboxlinux.hpp"
#include "opbox_software/opboxlogging.hpp"
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
        return thread->joinable();
    }


    void Subprocess::kill()
    {
        OPBOX_LOG_DEBUG("Killing program %s with SIGINT", program.c_str());
        if(pid > 0)
        {
            ::kill(pid, SIGINT);
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
        OPBOX_LOG_DEBUG("Forking and starting program %s in another process", program.c_str());

        pid = fork();
        if(pid == -1)
        {
            OPBOX_LOG_ERROR("Failed to fork(): %s", strerror(errno));
        }

        if(pid == 0)
        {
            //child process, assemble args into c style string array and start program
            char *execArgs[(const size_t) args.size() + 1];
            for(int i = 0; i < args.size(); i++)
            {
                execArgs[i] = (char*) malloc(args[i].length());
                strcpy(execArgs[i], args[i].c_str());
            }

            execArgs[args.size()] = (char*) 0;

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
            waitpid(pid, &status, 0); //wait for program to stop

            //handle return
            if(WIFEXITED(status))
            {
                retcode = WEXITSTATUS(status);
                handleProcessReturn(retcode);
            } else
            {
                //uh oh
                OPBOX_LOG_ERROR("waitpid() returned but program %s has not exited", program.c_str());
            }
        }
    }


    void Subprocess::defaultProcessReturnHandler(int retcode)
    {
        OPBOX_LOG_DEBUG("Program %s returned with code %d", program.c_str(), retcode);
    }
}
