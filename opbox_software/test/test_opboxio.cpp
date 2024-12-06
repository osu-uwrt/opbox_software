#include <gtest/gtest.h>
#include <cstdio>
#include "opboxio.hpp"

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

TEST(TestOpboxIO, TestIOActuator)
{
    GTEST_SKIP();
}