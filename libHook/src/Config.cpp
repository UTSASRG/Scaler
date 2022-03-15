#include <util/config/Config.h>

YAML::Node scaler::Config::globalConf = YAML::Node();
long scaler::Config::curJobId = -1;
pthread_t scaler::Config::mainthreadID = 0;

uint64_t scaler::Config::libHookStartingAddr = 0;
uint64_t scaler::Config::libHookEndingAddr = 0;
short scaler::Config::maximumHierachy = 1;
