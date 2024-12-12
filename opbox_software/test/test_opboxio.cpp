#include "opbox_software/opboxio.hpp"
#include <gtest/gtest.h>
#include <cstdio>

TEST(TestOpboxIO, TestInFile)
{
    opbox::StringInFile inFile("test_files/a.txt");
    ASSERT_TRUE(inFile.exists());
    ASSERT_EQ(inFile.read(), "A text");

    opbox::StringInFile inFile2("test_files/b.txt");
    ASSERT_TRUE(inFile2.exists());
    ASSERT_EQ(inFile2.read(), "1");

    opbox::StringInFile inFile3("test_files/asdf");
    ASSERT_FALSE(inFile3.exists());
    ASSERT_EQ(inFile3.read(), "");
}

#define OUTFILE_TEST_FILE "test_files/c.txt"
#define OUTFILE_TEST_DATA "hello!"

TEST(TestOpboxIO, TestOutFile)
{
    //test assumes that InFile works
    //ensure that file is deleted to test creation
    opbox::StringInFile f(OUTFILE_TEST_FILE);
    if(f.exists())
    {
        //remove file
        ASSERT_EQ(remove(OUTFILE_TEST_FILE), 0);
    }

    ASSERT_FALSE(f.exists());

    opbox::StringOutFile outFile(OUTFILE_TEST_FILE);
    outFile.append(OUTFILE_TEST_DATA);
    ASSERT_TRUE(f.exists());
    ASSERT_EQ(f.read(), OUTFILE_TEST_DATA);

    //test append
    outFile.append(OUTFILE_TEST_DATA);
    std::string nextExpectedResult = OUTFILE_TEST_DATA OUTFILE_TEST_DATA;
    ASSERT_EQ(f.read(), nextExpectedResult);

    //test truncate
    outFile.write(OUTFILE_TEST_DATA);
    ASSERT_EQ(f.read(), OUTFILE_TEST_DATA);
}

TEST(TestOpboxIO, TestIOActuatorInt)
{
    opbox::IOActuator<int> act(std::make_unique<opbox::OutFile<int>>(OUTFILE_TEST_FILE), 0);
    opbox::ActuatorPattern<int> patt = {
        {1, 400ms},
        {0, 300ms},
        {1, 200ms},
        {0, 100ms}
    };

    act.setPattern(patt);

    opbox::StringInFile f(OUTFILE_TEST_FILE);
    std::this_thread::sleep_for(25ms);
    ASSERT_EQ(f.read(), "1");
    ASSERT_TRUE(act.state());
    std::this_thread::sleep_for(400ms);
    ASSERT_EQ(f.read(), "0");
    ASSERT_FALSE(act.state());
    std::this_thread::sleep_for(300ms);
    ASSERT_EQ(f.read(), "1");
    ASSERT_TRUE(act.state());
    std::this_thread::sleep_for(200ms);
    ASSERT_EQ(f.read(), "0");
    ASSERT_FALSE(act.state());
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(f.read(), "1");
    ASSERT_TRUE(act.state());
}

TEST(TestOpboxIO, TestIOLedOff)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_OFF);
    opbox::StringInFile f(OPBOX_IO_BACKUP_LED_FILE);

    //check for 2 seconds that file has a 0 in it
    std::this_thread::sleep_for(100ms);
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(10ms);
        ASSERT_EQ(f.read(), "0");
    }
}

TEST(TestOpboxIO, TestIOLedOn)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_ON);
    opbox::StringInFile f(OPBOX_IO_BACKUP_LED_FILE);

    //check for 2 seconds that file has a 1 in it
    std::this_thread::sleep_for(100ms);
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.read(), "1");
    }
}



TEST(TestOpboxIO, TestIOBuzzerOff)
{
    opbox::IOBuzzer buzzer(true);
    buzzer.setState(opbox::IOBuzzerState::IO_BUZZER_OFF);
    opbox::StringInFile f(OPBOX_IO_BACKUP_BUZZER_FILE);

    std::this_thread::sleep_for(25ms);

    //check for 2 seconds that file has a 0 in it
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.read(), "0");
    }
}

TEST(TestOpboxIO, TestIOBuzzerChirp)
{
    opbox::IOBuzzer buzzer(true);
    buzzer.setState(opbox::IOBuzzerState::IO_BUZZER_CHIRP);
    opbox::StringInFile f(OPBOX_IO_BACKUP_BUZZER_FILE);

    std::this_thread::sleep_for(25ms);
    ASSERT_EQ(f.read(), "1");

    //check for 2 seconds that file has a 0 in it
    auto start = std::chrono::system_clock::now();
    while(std::chrono::system_clock::now() - start < 2s)
    {
        std::this_thread::sleep_for(300ms);
        ASSERT_EQ(f.read(), "0");
    }
}

TEST(TestOpboxIO, TestIOScheduling)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_ON);
    led.setNextState(opbox::IOLedState::IO_LED_OFF, 500ms);

    opbox::StringInFile f(OPBOX_IO_BACKUP_LED_FILE);
    
    //know that normal setting states work if others pass. For this test just schedule a change
    std::this_thread::sleep_for(400ms);
    ASSERT_EQ(f.read(), "1");
    std::this_thread::sleep_for(200ms);
    ASSERT_EQ(f.read(), "0");
    //test that ioact does not try to pull a nonexistent queue item after another 500ms
    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(f.read(), "0");
}

TEST(TestOpboxIO, TestIOSchedulingInterval)
{
    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_ON);
    led.setNextState(opbox::IOLedState::IO_LED_OFF, 500ms);
    led.setNextState(opbox::IOLedState::IO_LED_ON, 500ms);
    led.setNextState(opbox::IOLedState::IO_LED_OFF, 500ms);

    opbox::StringInFile f(OPBOX_IO_BACKUP_LED_FILE);
    
    //know that normal setting states work if others pass. For this test just schedule a change
    std::this_thread::sleep_for(250ms);
    ASSERT_EQ(f.read(), "1");
    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(f.read(), "0");
    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(f.read(), "1");
    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(f.read(), "0");
}

TEST(TestOpboxIO, TestIOQueueInterruptClearing)
{
    auto start = std::chrono::system_clock::now();

    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_ON);
    led.setNextState(opbox::IOLedState::IO_LED_OFF, 750ms);
    led.setNextState(opbox::IOLedState::IO_LED_ON, 250ms); //test that the queue is wiped. this should be cleared
    opbox::StringInFile f(OPBOX_IO_BACKUP_LED_FILE);
    
    //know that normal setting states work if others pass. For this test just schedule a change
    std::this_thread::sleep_for(400ms);
    ASSERT_EQ(f.read(), "1");
    led.setState(opbox::IOLedState::IO_LED_OFF);
    //time elapsed after this call should be 500ms, well under the delay time
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(f.read(), "0");

    auto end = std::chrono::system_clock::now();
    ASSERT_LT(end - start, 750ms);

    std::this_thread::sleep_for(500ms);
    ASSERT_EQ(f.read(), "0");
}

TEST(TestOpboxIO, TestIOQueueInterruptNonClearing)
{
    auto start = std::chrono::system_clock::now();

    opbox::IOLed led(true);
    led.setState(opbox::IOLedState::IO_LED_ON);
    led.setNextState(opbox::IOLedState::IO_LED_OFF, 750ms);
    led.setNextState(opbox::IOLedState::IO_LED_ON, 500ms); //test that the queue is wiped. this should be cleared
    opbox::StringInFile f(OPBOX_IO_BACKUP_LED_FILE);

    //know that normal setting states work if others pass. For this test just schedule a change
    std::this_thread::sleep_for(400ms);
    ASSERT_EQ(f.read(), "1");
    led.setState(opbox::IOLedState::IO_LED_OFF, false);
    //time elapsed after this call should be 500ms, well under the delay time
    std::this_thread::sleep_for(100ms);
    ASSERT_EQ(f.read(), "0");

    auto end = std::chrono::system_clock::now();
    ASSERT_LT(end - start, 750ms);

    std::this_thread::sleep_for(1s);
    ASSERT_EQ(f.read(), "1");
}

TEST(TestOpboxIO, TestGPIOOutput)
{
    opbox::GPIOOutput gpout(1, true, "./test_files/test_gpio");
    opbox::StringInFile f("./test_files/test_gpio");
    gpout.write(false);
    ASSERT_TRUE(f.exists());
    ASSERT_EQ(f.read(), "0");
    gpout.write(true);
    ASSERT_EQ(f.read(), "1");
}

TEST(TestOpboxIO, TestGPIOInput)
{
    opbox::GPIOInput gpin(1, true, "./test_files/test_gpio");
    opbox::StringOutFile f("./test_files/test_gpio");
    f.write("1");
    ASSERT_TRUE(gpin.read());
    f.write("0");
    ASSERT_FALSE(gpin.read());
}
