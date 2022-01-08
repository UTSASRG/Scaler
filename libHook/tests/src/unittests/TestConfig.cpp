#include <iostream>

#include <gtest/gtest.h>
#include <util/tool/Logging.h>
#include <util/tool/Config.h>
#include <fstream>
TEST(Config, readwriteconfig) {

    std::ofstream myFile;
    myFile.open("unittestconfig.ini");
    myFile<<"[libScalerHook]"<<std::endl;
    myFile<<"#======================================================================"<<std::endl;
    myFile<<"# Set detailed log for additional debugging info"<<std::endl;
    myFile<<"DetailedLog=1"<<std::endl;
    myFile<<"RunStatus=1"<<std::endl;
    myFile<<"StatusPort=6090"<<std::endl;
    myFile<<"StatusRefresh=10.11"<<std::endl;
    myFile<<"Archive=3.1415926"<<std::endl;
    myFile<<"# Sets the location of the MV_FTP log file"<<std::endl;
    myFile<<"LogFile=/opt/ecs/mvuser/MV_IPTel/log/MV_IPTel.log"<<std::endl;
    myFile<<"#======================================================================"<<std::endl;
    myFile<<""<<std::endl;
    myFile<<""<<std::endl;
    myFile.close();

    scaler::Config *config = scaler::Config::getInst("unittestconfig.ini");

    EXPECT_EQ(config->get<int>("libScalerHook","DetailedLog",-1),1);
    EXPECT_EQ(config->get<int>("libScalerHook","RunStatus",-1),1);
    EXPECT_EQ(config->get<int>("libScalerHook","StatusPort",-1),6090);
    EXPECT_EQ(config->get<std::string>("libScalerHook","LogFile",""),"/opt/ecs/mvuser/MV_IPTel/log/MV_IPTel.log");
    EXPECT_TRUE(config->get<float>("libScalerHook","StatusRefresh",-1)-10.11<0.001);
    EXPECT_TRUE(config->get<double>("libScalerHook","Archive",-1)-3.1415926<0.00000001);

}


