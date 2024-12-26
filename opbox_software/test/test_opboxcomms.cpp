#include <gtest/gtest.h>
#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

class OpboxRobotLinkTest : public ::testing::Test
{
    protected: 

    void SetUp() override 
    {
        robotLink = std::make_unique<opbox::RobotLink>(
            "localhost", 9000,
            std::bind(&OpboxRobotLinkTest::handleNotificationFromRobot, this, _1, _2, _3),
            std::bind(&OpboxRobotLinkTest::handleStatusFromRobot, this, _1, _2, _3, _4));

        opboxLink = std::make_unique<opbox::OpboxLink>(
            "localhost", 9000,
            std::bind(&OpboxRobotLinkTest::handleNotificationFromOpbox, this, _1, _2, _3),
            std::bind(&OpboxRobotLinkTest::handleKillButtonFromOpbox, this, _1));
    }


    void TearDown() override
    { }


    opbox::KillSwitchState getLatestKillButtonState()
    {
        return latestKillButtonState;
    }

    opbox::OpboxRobotLink::UniquePtr
        robotLink,
        opboxLink;

    private:

    void handleNotificationFromRobot(const opbox::NotificationType& type, const std::string& sensorName, const std::string& desc)
    {

    }


    void handleNotificationFromOpbox(const opbox::NotificationType& type, const std::string& sensorName, const std::string& desc)
    {

    }


    void handleStatusFromRobot(const opbox::KillSwitchState& robotKillState, const opbox::ThrusterState& thrusterState, const opbox::DiagnosticState& diagState, const opbox::LeakState& leakState)
    {

    }


    void handleKillButtonFromOpbox(const opbox::KillSwitchState& killButtonState)
    {
        latestKillButtonState = killButtonState;
    }

    opbox::KillSwitchState latestKillButtonState;

};


// TEST_F(OpboxRobotLinkTest, TestRobotLinkConnected)
// {
//     ASSERT_TRUE(robotLink->connected());
//     ASSERT_TRUE(opboxLink->connected());

//     //disconnect opbox link to test robot link connection
//     opboxLink.reset();

//     std::this_thread::sleep_for(250ms);
//     ASSERT_TRUE(robotLink->connected());

//     std::this_thread::sleep_for(300ms);
//     ASSERT_FALSE(robotLink->connected());
// }

// TEST_F(OpboxRobotLinkTest, TestOpboxLinkConnected)
// {
//     ASSERT_TRUE(robotLink->connected());
//     ASSERT_TRUE(opboxLink->connected());

//     //disconnect opbox link to test robot link connection
//     robotLink.reset();

//     std::this_thread::sleep_for(250ms);
//     ASSERT_TRUE(opboxLink->connected());

//     std::this_thread::sleep_for(300ms);
//     ASSERT_FALSE(opboxLink->connected());
// }

TEST_F(OpboxRobotLinkTest, TestSendKillButtonState)
{
    std::this_thread::sleep_for(1s);
    robotLink->sendKillButtonState(opbox::KillSwitchState::KILLED);
    // std::this_thread::sleep_for(50ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::KILLED);

    robotLink->sendKillButtonState(opbox::KillSwitchState::UNKILLED);
    // std::this_thread::sleep_for(50ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::UNKILLED);

    robotLink->sendKillButtonState(opbox::KillSwitchState::KILLED);
    // std::this_thread::sleep_for(50ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::KILLED);

    robotLink->sendKillButtonState(opbox::KillSwitchState::KILLED);
    // std::this_thread::sleep_for(50ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::KILLED);

    robotLink->sendKillButtonState(opbox::KillSwitchState::UNKILLED);
    // std::this_thread::sleep_for(50ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::UNKILLED);
}
