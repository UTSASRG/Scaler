
#ifndef SCALER_CONFIG_H
#define SCALER_CONFIG_H

#include <yaml-cpp/yaml.h>

namespace scaler {
    class Config {
    public:
        static YAML::Node globalConf;
        static long curJobId;
    };
}

#endif //SCALER_CONFIG_H
