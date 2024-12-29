#include "opbox_software/opboxlinux.hpp"

void handleReturnCode(int code)
{
    OPBOX_LOG_INFO("Received return code %d from alert.", code);
}

int main(int argc, char **argv)
{
    opbox::Subprocess sp(
        "opbox_alert",
        {
            "--header",
            "OpboxAlert test",
            "--body",
            "This is a test of the OpboxAlert",
            "--button1-text",
            "Press for return code 0",
            "--button2-text",
            "Press for return code 1",
            "--button2-enabled",
            "true"
        },

        &handleReturnCode
    );

    sp.run();
    sp.wait();

    OPBOX_LOG_INFO("Subprocess return code is %d", sp.returnCode());

    return 0;
}
