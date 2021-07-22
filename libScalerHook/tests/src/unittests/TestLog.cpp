#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>

TEST(Logging, printlog) {
    DBG_LOG("Test");
    DBG_LOGS("custom template log: %s","Test");
    ERR_LOG("Test");
    ERR_LOGS("custom template err: %s","Test");

}


