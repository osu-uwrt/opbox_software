#include <rclcpp/rclcpp.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <riptide_msgs2/msg/kill_switch_report.hpp>
#include <riptide_msgs2/srv/send_opbox_notification.hpp>
#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

#define KILL_BUTTON_TOPIC "stuff"
#define DIAGNOSTICS_TOPIC "/diagnostics"
#define NOTIFICATION_SRV_NAME "send_opbox_notification"

struct DangerState {
    std::string diagName;
    int8_t diagStatus;
    int16_t minMsgs;
    opbox::NotificationType opboxStatus;

    uint16_t _msgsReceived;
    bool _dangerStateReached;
};

typedef std::map<std::string, DangerState> DangerStateArray;
typedef riptide_msgs2::srv::SendOpboxNotification SendOpboxNotification;

class OpboxRosClient : public rclcpp::Node
{
    public:
    OpboxRosClient()
     : rclcpp::Node("opbox_ros_client"),
       dangerStates(readDangerStates())
    {
        //parameters (including danger states)
        declare_parameter<std::string>("opbox_address", "");
        declare_parameter<int>("opbox_port", 0);

        //comms
        std::string opboxAddress = get_parameter("opbox_address").as_string();
        int opboxPort = get_parameter("opbox_port").as_int();

        if(opboxAddress.empty() || opboxPort == 0)
        {
            RCLCPP_ERROR(get_logger(), 
                "Required parameters not set! Address (\"%s\") is empty or port (%d) is zero",
                opboxAddress.c_str(), opboxPort);
            
            rclcpp::shutdown();
            exit(1);
        }

        RCLCPP_INFO(get_logger(), "Constructing robot link on address %s and port %d", opboxAddress.c_str(), opboxPort);
        opboxLink = std::make_unique<opbox::OpboxLink>(
            opboxAddress,
            opboxPort,
            std::bind(&OpboxRosClient::handleNotification, this, _1, _2, _3),
            std::bind(&OpboxRosClient::handleNewKillButtonState, this, _1),
            std::bind(&OpboxRosClient::handleNewConnectionState, this, _1));
        
        //ROS stuff
        killButtonPublisher = create_publisher<riptide_msgs2::msg::KillSwitchReport>(KILL_BUTTON_TOPIC, 10);
        diagnosticSub = create_subscription<diagnostic_msgs::msg::DiagnosticArray>(
            DIAGNOSTICS_TOPIC, 10, std::bind(&OpboxRosClient::diagStatusCb, this, _1));

        sendOpboxNotificationSrv = create_service<SendOpboxNotification>(
            NOTIFICATION_SRV_NAME,
            std::bind(&OpboxRosClient::sendNotificationToOpbox, this, _1, _2));

        RCLCPP_INFO(get_logger(), "Opbox ROS client started.");
    }

    private:

    DangerStateArray readDangerStates(void)
    {
        OPBOX_LOG_DEBUG("Reading diagnostic states");
        DangerStateArray dangerStates;
        int dangerStateIdx = 0;
        
        bool
            haveName = true,
            haveStatus = true,
            haveMinNumMsgs = true,
            haveOpboxStatus = true,
            haveAllParams = true;

        while(haveAllParams)
        {
            std::string 
                dangerStateName = "dangerstate" + std::to_string(dangerStateIdx),
                diagnosticNameParam = "danger_states." + dangerStateName + ".diagnostic_name",
                diagnosticStatusParam = "danger_states." + dangerStateName + ".diagnostic_status",
                minNumMsgsParam = "danger_states." + dangerStateName + ".minimum_num_msgs",
                opboxStatusParam = "danger_states." + dangerStateName + ".opbox_status";

            if(!has_parameter(diagnosticNameParam))
            {
                declare_parameter<std::string>(diagnosticNameParam, "");
            }

            if(!has_parameter(diagnosticStatusParam))
            {
                declare_parameter<int>(diagnosticStatusParam, -1);
            }

            if(!has_parameter(minNumMsgsParam))
            {
                declare_parameter<int>(minNumMsgsParam, -1);
            }

            if(!has_parameter(opboxStatusParam))
            {
                declare_parameter<int>(opboxStatusParam, -1);
            }

            DangerState ds;
            ds.diagName = get_parameter(diagnosticNameParam).as_string(),
            ds.diagStatus = get_parameter(diagnosticStatusParam).as_int(),
            ds.minMsgs = get_parameter(minNumMsgsParam).as_int(),
            ds.opboxStatus = (opbox::NotificationType) get_parameter(opboxStatusParam).as_int();
            ds._dangerStateReached = false;
            ds._msgsReceived = 0;
            
            haveName = !ds.diagName.empty();
            haveStatus = ds.diagStatus != -1;
            haveMinNumMsgs = ds.minMsgs != -1;
            haveOpboxStatus = ds.opboxStatus != -1;
            haveAllParams = haveName && haveStatus && haveMinNumMsgs && haveOpboxStatus;

            OPBOX_LOG_DEBUG("Have params for danger state %d: %s. name: %s, status: %s, minMsgs: %s, opboxStatus: %s", 
                dangerStateIdx, 
                haveAllParams ? "yes" : "no",
                haveName ? "yes" : "no",
                haveStatus ? "yes" : "no",
                haveMinNumMsgs ? "yes" : "no",
                haveOpboxStatus ? "yes" : "no");
            
            if(haveAllParams)
            {
                OPBOX_LOG_DEBUG("Read danger state %s, %d, %d, %s", 
                    ds.diagName.c_str(), 
                    ds.diagStatus, 
                    ds.minMsgs, 
                    opbox::notificationTypeToString(ds.opboxStatus).c_str());
                
                dangerStates.insert({ds.diagName, ds});
            }

            dangerStateIdx++;
        }

        OPBOX_LOG_DEBUG("Read %d danger states", dangerStates.size());
        return dangerStates;
    }

    void handleNotification(const opbox::NotificationType& type, const std::string& sensor, const std::string& desc)
    {
        RCLCPP_INFO(get_logger(), 
            "Notification with type %d received from sensor %s with description %s", 
            type, sensor.c_str(), desc.c_str());
    }

    void handleNewKillButtonState(const opbox::KillSwitchState& killButtonState)
    {
        riptide_msgs2::msg::KillSwitchReport ksr;
        ksr.sender_id = riptide_msgs2::msg::KillSwitchReport::KILL_SWITCH_TOPSIDE_BUTTON;
        ksr.switch_asserting_kill = killButtonState == opbox::KillSwitchState::KILLED;
        ksr.switch_needs_update = false;
        killButtonPublisher->publish(ksr);
    }

    void handleNewConnectionState(const bool& connected)
    {
        RCLCPP_INFO(get_logger(),
            "Opbox host %s", (connected ? "connected" : "not connected"));
    }

    void sendNotificationToOpbox(
        const std::shared_ptr<SendOpboxNotification::Request> request,
        std::shared_ptr<SendOpboxNotification::Response> response)
    {
        if(opboxLink->connected())
        {
            response->success = opboxLink->sendNotification((opbox::NotificationType) request->severity, request->sensor, request->description);
            response->message = (response->success ? 
                "Success" : "Opbox is connected, but notification was not acknowledged.");
        } else
        {
            response->success = false;
            response->message = "Cannot send notification because opbox is not connected.";
        }
    }

    void diagStatusCb(const diagnostic_msgs::msg::DiagnosticArray::SharedPtr msg)
    {
        for(size_t i = 0; i < msg->status.size(); i++)
        {
            //does this diagnostic name exist in danger states?
            diagnostic_msgs::msg::DiagnosticStatus status = msg->status[i];
            if(dangerStates.count(status.name) > 0)
            {
                //now check to see if danger state is reached
                if(status.level == dangerStates.at(status.name).diagStatus)
                {
                    dangerStates.at(status.name)._msgsReceived++;
                    OPBOX_LOG_DEBUG("Received %d consecutive danger messages from %s (minimum %d, already reached: %s)", 
                        dangerStates.at(status.name)._msgsReceived, status.name.c_str(), dangerStates.at(status.name).minMsgs, dangerStates.at(status.name)._dangerStateReached ? "yes" : "no");

                    if(dangerStates.at(status.name)._msgsReceived >= dangerStates.at(status.name).minMsgs && !dangerStates.at(status.name)._dangerStateReached)
                    {
                        //danger state reached, ping opbox
                        OPBOX_LOG_ERROR("Danger state %s %d reached! Sending notification", status.name, status.level);
                        if(!opboxLink->sendNotification(dangerStates.at(status.name).opboxStatus, status.name.c_str(), status.message))
                        {
                            OPBOX_LOG_ERROR("FAILED TO SEND DANGER NOTIFICATION TO OPBOX");
                        }
                        dangerStates.at(status.name)._dangerStateReached = true;
                    }
                } else
                {
                    dangerStates.at(status.name)._dangerStateReached = false;
                    dangerStates.at(status.name)._msgsReceived = 0;
                }
            }
        }
    }

    DangerStateArray dangerStates;
    opbox::OpboxLink::UniquePtr opboxLink;
    rclcpp::Subscription<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diagnosticSub;
    rclcpp::Publisher<riptide_msgs2::msg::KillSwitchReport>::SharedPtr killButtonPublisher;
    rclcpp::Service<SendOpboxNotification>::SharedPtr sendOpboxNotificationSrv;
};


int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::Node::SharedPtr client = std::make_shared<OpboxRosClient>();
    rclcpp::spin(client);
    rclcpp::shutdown();
    return 0;
}
