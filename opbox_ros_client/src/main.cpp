#include <rclcpp/rclcpp.hpp>
#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

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
        RCLCPP_INFO(get_logger(),
            "New kill button state %d received", killButtonState);
    }

    void handleNewConnectionState(const bool& connected)
    {
        RCLCPP_INFO(get_logger(),
            "Opbox host %s", (connected ? "connected" : "not connected"));
    }

    opbox::OpboxLink::UniquePtr opboxLink;
};


int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::Node::SharedPtr client = std::make_shared<OpboxRosClient>();
    rclcpp::spin(client);
    rclcpp::shutdown();
    return 0;
}
