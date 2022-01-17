#include <util/config/Config.h>

YAML::Node scaler::Config::globalConf=YAML::Node();
long scaler::Config::curJobId=-1;
pthread_t scaler::Config::mainthreadID=0;

