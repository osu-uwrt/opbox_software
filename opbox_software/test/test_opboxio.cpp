#include "opbox_software/opboxio.hpp"
#include <gtest/gtest.h>
#include <cstdio>

TEST(TestOpboxIO, TestInFile)
{
    opbox::InFile inFile("test_files/a.txt");
    ASSERT_TRUE(inFile.exists());
    ASSERT_EQ(inFile.readFile(), "A text");

    opbox::InFile inFile2("test_files/b.txt");
    ASSERT_TRUE(inFile2.exists());
    ASSERT_EQ(inFile2.readFile(), "1");

    opbox::InFile inFile3("test_files/asdf");
    ASSERT_FALSE(inFile3.exists());
    ASSERT_EQ(inFile3.readFile(), "");
}

#define OUTFILE_TEST_FILE "test_files/c.txt"
#define OUTFILE_TEST_DATA "hello!"

TEST(TestOpboxIO, TestOutFile)
{
    //test assumes that InFile works
    //ensure that file is deleted to test creation
    opbox::InFile f(OUTFILE_TEST_FILE);
    if(f.exists())
    {
        //remove file
        ASSERT_EQ(remove(OUTFILE_TEST_FILE), 0);
    }

    ASSERT_FALSE(f.exists());

    opbox::OutFile outFile(OUTFILE_TEST_FILE);
    outFile.appendToFile(OUTFILE_TEST_DATA);
    ASSERT_TRUE(f.exists());
    ASSERT_EQ(f.readFile(), OUTFILE_TEST_DATA);

    //test append
    outFile.appendToFile(OUTFILE_TEST_DATA);
    std::string nextExpectedResult = OUTFILE_TEST_DATA OUTFILE_TEST_DATA;
    ASSERT_EQ(f.readFile(), nextExpectedResult);

    //test truncate
    outFile.replaceFile(OUTFILE_TEST_DATA);
    ASSERT_EQ(f.readFile(), OUTFILE_TEST_DATA);
}

TEST(TestOpboxIO, TestIOActuatorInt)
{
    opbox::IOActuator<int> act(OUTFILE_TEST_FILE, 0);
    opbox::ActuatorPattern<int> patt = {
        {1, 400ms},
        {0, 100ms},
        {1, 200ms},
        {0, 100ms}
    };

    act.setPattern(patt);

    opbox::InFile f(OUTFILE_TEST_FILE);
    std::this_thread::sleep_for(25ms);
    ASSERT_EQ(f.readFile(), "1");
    ASSERT_TRUE(act.state());
    std::this_thread::sleep_for(400ms);
    ASSERT_EQ(f.readFile(), "0");
    ASSERT_FALSE(act.state());
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(f.readFile(), "1");
    ASSERT_TRUE(act.state());
    std::this_thread::sleep_for(200ms);
    ASSERT_EQ(f.readFile(), "0");
    ASSERT_FALSE(act.state());
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(f.readFile(), "1");
    ASSERT_TRUE(act.state());
}

TEST(TestOpboxIO, TestIOLedOff)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_OFF);
    opbox::InFile f(OPBOX_IO_BACKUP_LED_FILE);

    //check for 2 seconds that file has a 0 in it
    std::this_thread::sleep_for(100ms);
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(10ms);
        ASSERT_EQ(f.readFile(), "0");
    }
}

TEST(TestOpboxIO, TestIOLedOn)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_ON);
    opbox::InFile f(OPBOX_IO_BACKUP_LED_FILE);

    //check for 2 seconds that file has a 1 in it
    std::this_thread::sleep_for(100ms);
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.readFile(), "1");
    }
}

TEST(TestOpboxIO, TestIOLedBlinkTwice)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_BLINK_TWICE);
    opbox::InFile f(OPBOX_IO_BACKUP_LED_FILE);

    std::this_thread::sleep_for(25ms);
    ASSERT_EQ(f.readFile(), "1");
    std::this_thread::sleep_for(125ms);
    ASSERT_EQ(f.readFile(), "0");
    std::this_thread::sleep_for(125ms);
    ASSERT_EQ(f.readFile(), "1");
    std::this_thread::sleep_for(125ms);
    
    //check for 2 seconds that file has a 0 in it
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.readFile(), "0");
    }
}

TEST(TestOpboxIO, TestIOLedBlinking)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_BLINKING);
    opbox::InFile f(OPBOX_IO_BACKUP_LED_FILE);

    std::this_thread::sleep_for(125ms);
    ASSERT_EQ(f.readFile(), "1");
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(250ms);
        ASSERT_EQ(f.readFile(), "0");
        std::this_thread::sleep_for(250ms);
        ASSERT_EQ(f.readFile(), "1");
    }
}

TEST(TestOpboxIO, TestIOBuzzerOff)
{
    opbox::IOBuzzer buzzer(true);
    buzzer.setState(opbox::IOBuzzerState::IO_BUZZER_OFF);
    opbox::InFile f(OPBOX_IO_BACKUP_BUZZER_FILE);

    std::this_thread::sleep_for(25ms);

    //check for 2 seconds that file has a 0 in it
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.readFile(), "0");
    }
}

TEST(TestOpboxIO, TestIOBuzzerChirp)
{
    opbox::IOBuzzer buzzer(true);
    buzzer.setState(opbox::IOBuzzerState::IO_BUZZER_CHIRP);
    opbox::InFile f(OPBOX_IO_BACKUP_BUZZER_FILE);

    std::this_thread::sleep_for(25ms);
    ASSERT_EQ(f.readFile(), "1");

    //check for 2 seconds that file has a 0 in it
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.readFile(), "0");
    }
}

TEST(TestOpboxIO, TestIOBuzzerChirpTwice)
{
    opbox::IOBuzzer buzzer(true);
    buzzer.setState(opbox::IOBuzzerState::IO_BUZZER_CHIRP_TWICE);
    opbox::InFile f(OPBOX_IO_BACKUP_BUZZER_FILE);

    std::this_thread::sleep_for(25ms);
    ASSERT_EQ(f.readFile(), "1");
    std::this_thread::sleep_for(125ms);
    ASSERT_EQ(f.readFile(), "0");
    std::this_thread::sleep_for(125ms);
    ASSERT_EQ(f.readFile(), "1");

    //check for 2 seconds that file has a 0 in it
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.readFile(), "0");
    }
}
