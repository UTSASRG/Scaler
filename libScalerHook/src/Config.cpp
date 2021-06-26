//
// Created by steven on 6/26/21.
//

#include <c++/7/string>
#include <util/tool/Config.h>
#include <c++/7/sstream>

scaler::Config *scaler::Config::instance = nullptr;

scaler::Config::Config(std::string fileName) : fileName(fileName) {
    reader = new INIReader(fileName);
    if (reader->ParseError() == -1) {
        std::stringstream ss;
        ss << "Config parsing failed for Path=" << this->fileName;
        throwScalerException(ss.str().c_str());
    }

}

scaler::Config *scaler::Config::getInst(std::string fileName) {
    if (instance == nullptr) {
        instance = new Config(fileName);
        return instance;
    } else {
        return instance;
    }
}

const std::set<std::string> &scaler::Config::sections() const {
    return reader->Sections();
}

template<>
std::string scaler::Config::get(std::string section, std::string name, std::string default_value) const {
    std::string rlt = reader->Get(section, name, default_value);
    return rlt;
}


template<>
bool scaler::Config::get(std::string section, std::string name, bool default_value) const {
    return reader->GetBoolean(section, name, default_value);
}

template<>
int scaler::Config::get(std::string section, std::string name, int default_value) const {
    return reader->GetInteger(section, name, default_value);
}

template<>
double scaler::Config::get(std::string section, std::string name, double default_value) const {
    return reader->GetReal(section, name, default_value);
}

template<>
float scaler::Config::get(std::string section, std::string name, float default_value) const {
    return reader->GetFloat(section, name, default_value);
}

