#include <rclcpp/rclcpp.hpp>
#include <riptide_msgs2/msg/kill_switch_report.hpp>
#include <riptide_msgs2/srv/send_opbox_notification.hpp>
#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

#define KILL_BUTTON_TOPIC "stuff"
#define NOTIFICATION_SRV_NAME "send_opbox_notification"

typedef riptide_msgs2::srv::SendOpboxNotification SendOpboxNotification;

class OpboxRosClient : public rclcpp::Node
{
    public:
    OpboxRosClient()
     : rclcpp::Node("opbox_ros_client")
    {
        //parameters
        declare_parameter<std::string>("opbox_address", "");
        declare_parameter<int>("opbox_port", 0);

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

        sendOpboxNotificationSrv = create_service<SendOpboxNotification>(
            NOTIFICATION_SRV_NAME,
            std::bind(&OpboxRosClient::sendNotificationToOpbox, this, _1, _2));


        RCLCPP_INFO(get_logger(), "Opbox ROS client started.");
    }

    private:

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


    opbox::OpboxLink::UniquePtr opboxLink;
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
