
#ifndef SCALER_CONFIG_H
#define SCALER_CONFIG_H

#include <yaml-cpp/yaml.h>
namespace scaler {
    class HookConfig {
    public:
        enum Mode {
            NORMAL = 0,
            COUNTING = 1,
        };

        HookConfig();

        bool parse(YAML::Node &config);

        std::string hook_gccpath;
        std::string analyzerserv_address;
        bool analyzerserv_insecure;
        Mode mode;
        bool logging_error;
        bool logging_info;
    };
}

namespace YAML {
    template<>
    struct convert<scaler::HookConfig> {
        static Node encode(const scaler::HookConfig &rhs) {
            Node node;
//            node.push_back(rhs.x);
//            node.push_back(rhs.y);
//            node.push_back(rhs.z);
            return node;
        }

        static bool decode(const Node &node, scaler::HookConfig &rhs) {
//            rhs.hook_gccpath = node["hook"]["gccpath"].as<std::string>("gcc");
//            rhs.analyzerserv_address = node["analyzerserv"]["address"].as<std::string>("127.0.0.1:3060");
//            rhs.analyzerserv_insecure = node["analyzerserv"]["insecure"].as<bool>(false);
//            std::string mode = node["mode"].as<std::string>("normal");
//            if (mode == "normal") {
//                rhs.mode = scaler::HookConfig::Mode::NORMAL;
//            } else if (mode == "counting") {
//                rhs.mode = scaler::HookConfig::Mode::COUNTING;
//            } else {
//                return false;
//            }
//            rhs.logging_error = node["logging"]["error"].as<bool>(true);
//            rhs.logging_info = node["logging"]["info"].as<bool>(true);
            return true;
        }
    };
}
#endif //SCALER_CONFIG_H
