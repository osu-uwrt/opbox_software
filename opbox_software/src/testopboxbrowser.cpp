#include "opbox_software/opboxlinux.hpp"

int main(int argc, char **argv)
{
    opbox::Subprocess sp("/usr/bin/sensible-browser");
    sp.run();
    OPBOX_LOG_INFO("Stopping browser in 5 seconds");
    std::this_thread::sleep_for(5s);
    OPBOX_LOG_INFO("Killing process");
    sp.kill();
    OPBOX_LOG_INFO("Browser process ended with code %d", sp.returnCode());
    return 0;
}
