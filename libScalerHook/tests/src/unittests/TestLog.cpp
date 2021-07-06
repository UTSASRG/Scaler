#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>

TEST(Logging, printlog) {
    DBG_LOG("Test\n")
    DBG_LOGS("custom template log: %s\n","Test")
    ERR_LOG("Test\n")
    ERR_LOGS("custom template err: %s\n","Test")

}


