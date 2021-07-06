#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Timer.h>
#include <thread>

TEST(Tools, getunixtimestampms) {
    auto start=getunixtimestampms();
    std::this_thread::sleep_for (std::chrono::seconds(1));
    auto end=getunixtimestampms();
    EXPECT_TRUE(abs(end-start)/1000000 - 1 < 0.00000001);

}




