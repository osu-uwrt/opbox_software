#include <gtest/gtest.h>
#include "opbox_software/opboxcomms.hpp"

using namespace std::placeholders;

struct RobotState
{
    opbox::KillSwitchState robotKillState = opbox::KillSwitchState::KILLED;
    opbox::ThrusterState thrusterState = opbox::ThrusterState::IDLE;
    opbox::DiagnosticState diagState = opbox::DiagnosticState::DIAGNOSTICS_OK;
    opbox::LeakState leakState = opbox::LeakState::OK;
};

struct Notification
{
    opbox::NotificationType type = opbox::NotificationType::NOTIFICATION_WARNING;
    std::string sensorName = "";
    std::string desc = "";
};

class OpboxRobotLinkTest : public ::testing::Test
{
    protected: 

    void SetUp() override 
    {
        robotLink = std::make_unique<opbox::RobotLink>(
            "localhost", 9000,
            std::bind(&OpboxRobotLinkTest::handleNotificationFromRobot, this, _1, _2, _3),
            std::bind(&OpboxRobotLinkTest::handleStatusFromRobot, this, _1, _2, _3, _4),
            std::bind(&OpboxRobotLinkTest::handleConnectionStateChange, this, _1));

        opboxLink = std::make_unique<opbox::OpboxLink>(
            "localhost", 9000,
            std::bind(&OpboxRobotLinkTest::handleNotificationFromOpbox, this, _1, _2, _3),
            std::bind(&OpboxRobotLinkTest::handleKillButtonFromOpbox, this, _1),
            std::bind(&OpboxRobotLinkTest::handleConnectionStateChange, this, _1));
    }


    void TearDown() override
    { }


    Notification getLatestNotificationFromRobot()
    {
        return latestNotificationFromRobot;
    }


    Notification getLatestNotificationFromOpbox()
    {
        return latestNotificationFromOpbox;
    }


    RobotState getLatestRobotState()
    {
        return latestRobotState;
    }


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
        latestNotificationFromRobot.type = type;
        latestNotificationFromRobot.sensorName = sensorName;
        latestNotificationFromRobot.desc = desc;
    }


    void handleNotificationFromOpbox(const opbox::NotificationType& type, const std::string& sensorName, const std::string& desc)
    {
        latestNotificationFromOpbox.type = type;
        latestNotificationFromOpbox.sensorName = sensorName;
        latestNotificationFromOpbox.desc = desc;
    }


    void handleStatusFromRobot(const opbox::KillSwitchState& robotKillState, const opbox::ThrusterState& thrusterState, const opbox::DiagnosticState& diagState, const opbox::LeakState& leakState)
    {
        latestRobotState.robotKillState = robotKillState;
        latestRobotState.thrusterState = thrusterState;
        latestRobotState.diagState = diagState;
        latestRobotState.leakState = leakState;
    }


    void handleKillButtonFromOpbox(const opbox::KillSwitchState& killButtonState)
    {
        OPBOX_LOG_DEBUG("Kill button state %d received", killButtonState);
        latestKillButtonState = killButtonState;
    }

    void handleConnectionStateChange(const bool& connected) { }

    Notification
        latestNotificationFromRobot,
        latestNotificationFromOpbox;

    RobotState latestRobotState;
    opbox::KillSwitchState latestKillButtonState;
};


TEST_F(OpboxRobotLinkTest, TestRobotLinkConnected)
{
    ASSERT_TRUE(robotLink->connected());
    ASSERT_TRUE(opboxLink->connected());

    std::this_thread::sleep_for(600ms);

    ASSERT_TRUE(robotLink->connected());
    ASSERT_TRUE(opboxLink->connected());

    //disconnect opbox link to test robot link connection
    opboxLink.reset();

    std::this_thread::sleep_for(250ms);
    ASSERT_TRUE(robotLink->connected());

    std::this_thread::sleep_for(300ms);
    ASSERT_FALSE(robotLink->connected());
}

TEST_F(OpboxRobotLinkTest, TestOpboxLinkConnected)
{
    ASSERT_TRUE(robotLink->connected());
    ASSERT_TRUE(opboxLink->connected());

    std::this_thread::sleep_for(600ms);

    ASSERT_TRUE(robotLink->connected());
    ASSERT_TRUE(opboxLink->connected());

    //disconnect opbox link to test robot link connection
    robotLink.reset();

    std::this_thread::sleep_for(250ms);
    ASSERT_TRUE(opboxLink->connected());

    std::this_thread::sleep_for(300ms);
    ASSERT_FALSE(opboxLink->connected());
}

TEST_F(OpboxRobotLinkTest, TestOpboxRobotLinkExceptions)
{
    ASSERT_THROW(opboxLink->sendKillButtonState(opbox::KillSwitchState::KILLED), std::runtime_error);
    ASSERT_THROW(robotLink->sendRobotState(
        opbox::KillSwitchState::KILLED,
        opbox::ThrusterState::IDLE,
        opbox::DiagnosticState::DIAGNOSTICS_OK,
        opbox::LeakState::OK), std::runtime_error);
}

TEST_F(OpboxRobotLinkTest, TestSendKillButtonState)
{
    OPBOX_LOG_DEBUG("Testing button state killed");
    robotLink->sendKillButtonState(opbox::KillSwitchState::KILLED);
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::KILLED);

    OPBOX_LOG_DEBUG("Testing button state unkilled");
    robotLink->sendKillButtonState(opbox::KillSwitchState::UNKILLED);
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::UNKILLED);

    OPBOX_LOG_DEBUG("Testing button state killed");
    robotLink->sendKillButtonState(opbox::KillSwitchState::KILLED);
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::KILLED);

    OPBOX_LOG_DEBUG("Testing button state killed");
    robotLink->sendKillButtonState(opbox::KillSwitchState::KILLED);
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::KILLED);

    OPBOX_LOG_DEBUG("Testing button state unkilled");
    robotLink->sendKillButtonState(opbox::KillSwitchState::UNKILLED);
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::UNKILLED);
}

TEST_F(OpboxRobotLinkTest, TestSendRobotState)
{
    OPBOX_LOG_DEBUG("Testing unkilled, active, warn, leaking");
    opboxLink->sendRobotState(
            opbox::KillSwitchState::UNKILLED, 
            opbox::ThrusterState::ACTIVE,
            opbox::DiagnosticState::DIAGNOSTICS_WARN,
            opbox::LeakState::LEAKING);
    
    std::this_thread::sleep_for(250ms);
    RobotState rs = getLatestRobotState();

    ASSERT_EQ(rs.robotKillState, opbox::KillSwitchState::UNKILLED);
    ASSERT_EQ(rs.thrusterState, opbox::ThrusterState::ACTIVE);
    ASSERT_EQ(rs.diagState, opbox::DiagnosticState::DIAGNOSTICS_WARN);
    ASSERT_EQ(rs.leakState, opbox::LeakState::LEAKING);


    OPBOX_LOG_DEBUG("Testing killed, idle, error, leaking");
    opboxLink->sendRobotState(
            opbox::KillSwitchState::KILLED, 
            opbox::ThrusterState::IDLE,
            opbox::DiagnosticState::DIAGNOSTICS_ERROR,
            opbox::LeakState::LEAKING);
    
    std::this_thread::sleep_for(250ms);
    rs = getLatestRobotState();

    ASSERT_EQ(rs.robotKillState, opbox::KillSwitchState::KILLED);
    ASSERT_EQ(rs.thrusterState, opbox::ThrusterState::IDLE);
    ASSERT_EQ(rs.diagState, opbox::DiagnosticState::DIAGNOSTICS_ERROR);
    ASSERT_EQ(rs.leakState, opbox::LeakState::LEAKING);


    OPBOX_LOG_DEBUG("Testing killed, idle, ok, ok");
    opboxLink->sendRobotState(
            opbox::KillSwitchState::KILLED, 
            opbox::ThrusterState::IDLE,
            opbox::DiagnosticState::DIAGNOSTICS_OK,
            opbox::LeakState::OK);
    
    std::this_thread::sleep_for(250ms);
    rs = getLatestRobotState();

    ASSERT_EQ(rs.robotKillState, opbox::KillSwitchState::KILLED);
    ASSERT_EQ(rs.thrusterState, opbox::ThrusterState::IDLE);
    ASSERT_EQ(rs.diagState, opbox::DiagnosticState::DIAGNOSTICS_OK);
    ASSERT_EQ(rs.leakState, opbox::LeakState::OK);
}

TEST_F(OpboxRobotLinkTest, TestPersistence)
{
    OPBOX_LOG_DEBUG("Testing button state unkilled");
    robotLink->sendKillButtonState(opbox::KillSwitchState::UNKILLED);
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::UNKILLED);


    OPBOX_LOG_DEBUG("Testing unkilled, active, warn, leaking");
    opboxLink->sendRobotState(
            opbox::KillSwitchState::UNKILLED, 
            opbox::ThrusterState::ACTIVE,
            opbox::DiagnosticState::DIAGNOSTICS_WARN,
            opbox::LeakState::LEAKING);
    
    std::this_thread::sleep_for(250ms);
    RobotState rs = getLatestRobotState();

    ASSERT_EQ(rs.robotKillState, opbox::KillSwitchState::UNKILLED);
    ASSERT_EQ(rs.thrusterState, opbox::ThrusterState::ACTIVE);
    ASSERT_EQ(rs.diagState, opbox::DiagnosticState::DIAGNOSTICS_WARN);
    ASSERT_EQ(rs.leakState, opbox::LeakState::LEAKING);

    std::this_thread::sleep_for(5s);
    ASSERT_EQ(getLatestKillButtonState(), opbox::KillSwitchState::UNKILLED);

    rs = getLatestRobotState();
    ASSERT_EQ(rs.robotKillState, opbox::KillSwitchState::UNKILLED);
    ASSERT_EQ(rs.thrusterState, opbox::ThrusterState::ACTIVE);
    ASSERT_EQ(rs.diagState, opbox::DiagnosticState::DIAGNOSTICS_WARN);
    ASSERT_EQ(rs.leakState, opbox::LeakState::LEAKING);
}

TEST_F(OpboxRobotLinkTest, TestNofificationSuccess)
{
    // GTEST_SKIP();
    OPBOX_LOG_DEBUG("Testing sucessful warn from robot");
    ASSERT_TRUE(opboxLink->sendNotification(opbox::NotificationType::NOTIFICATION_WARNING, "test", "test description"));
    Notification notification = getLatestNotificationFromRobot();
    ASSERT_EQ(notification.type, opbox::NotificationType::NOTIFICATION_WARNING);
    ASSERT_EQ(notification.sensorName, "test");
    ASSERT_EQ(notification.desc, "test description");

    OPBOX_LOG_DEBUG("Testing sucessful error from robot");
    ASSERT_TRUE(opboxLink->sendNotification(opbox::NotificationType::NOTIFICATION_ERROR, "sensor", "some error"));
    notification = getLatestNotificationFromRobot();
    ASSERT_EQ(notification.type, opbox::NotificationType::NOTIFICATION_ERROR);
    ASSERT_EQ(notification.sensorName, "sensor");
    ASSERT_EQ(notification.desc, "some error");

    OPBOX_LOG_DEBUG("Testing sucessful warn from opbox");
    ASSERT_TRUE(robotLink->sendNotification(opbox::NotificationType::NOTIFICATION_WARNING, "opbox", "something"));
    notification = getLatestNotificationFromOpbox();
    ASSERT_EQ(notification.type, opbox::NotificationType::NOTIFICATION_WARNING);
    ASSERT_EQ(notification.sensorName, "opbox");
    ASSERT_EQ(notification.desc, "something");

    OPBOX_LOG_DEBUG("Testing sucessful fatal from opbox");
    ASSERT_TRUE(robotLink->sendNotification(opbox::NotificationType::NOTIFICATION_FATAL, "opbox", "fatal issue"));
    notification = getLatestNotificationFromOpbox();
    ASSERT_EQ(notification.type, opbox::NotificationType::NOTIFICATION_FATAL);
    ASSERT_EQ(notification.sensorName, "opbox");
    ASSERT_EQ(notification.desc, "fatal issue");
}

TEST_F(OpboxRobotLinkTest, TestNotificationFailure)
{
    OPBOX_LOG_DEBUG("Testing failing fatal from opbox");
    opboxLink.reset();
    ASSERT_FALSE(robotLink->sendNotification(opbox::NotificationType::NOTIFICATION_FATAL, "opbox", "fatal issue"));
}
