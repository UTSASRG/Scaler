//
// Created by steven on 6/26/21.
//

#ifndef SCALER_CONFIG_H
#define SCALER_CONFIG_H

#include <exceptions/ScalerException.h>
#include "INIReader.h"

namespace scaler {
    class Config {
    public:


        static Config *instance;

        //MemoryTool_Linux::instance memory leak

        static Config *getInst(std::string fileName = "ScalerConfig.ini");

        // Return the list of sections found in ini file
        const std::set<std::string> &sections() const;

        // Get a string value from INI file, returning default_value if not found.
        template<typename CONFIG_TYPE>
        CONFIG_TYPE get(std::string section, std::string name, CONFIG_TYPE default_value) const {
            throwScalerException(ErrCode::FUNC_NOT_IMPLEMENTED, "Function not implemented");
        }

    protected:

        Config(std::string fileName);

        INIReader *reader;

        std::string fileName;
    };


}

template<>
std::string scaler::Config::get(std::string section, std::string name, std::string default_value) const;

template<>
bool scaler::Config::get(std::string section, std::string name, bool default_value) const;

template<>
int scaler::Config::get(std::string section, std::string name, int default_value) const;

template<>
double scaler::Config::get(std::string section, std::string name, double default_value) const;

template<>
float scaler::Config::get(std::string section, std::string name, float default_value) const;

#endif //SCALER_CONFIG_H
