#include <iostream>

#include <gtest/gtest.h>
#include <util/hook/ExtFuncCallHookAsm.hh>
#include <exceptions/ScalerException.h>

using namespace std;
using namespace scaler;

TEST(Exception, throwException) {
    try {
        throwScalerException("Hello");
    } catch (ScalerException &excet) {
        EXPECT_EQ(std::string(excet.what()), std::string("Hello"));
    }
}


