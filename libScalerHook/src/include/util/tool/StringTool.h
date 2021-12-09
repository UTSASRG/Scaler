#ifndef SCALER_STRTOOL_H
#define SCALER_STRTOOL_H


#include <cstddef>
#include <cstdio>
#include <vector>
#include <sstream>

namespace scaler {

    bool strEndsWith(std::string const &fullString, std::string const &ending);

    bool strStartsWith(std::string const &fullString, std::string const &ending);

    bool strContains(std::string const &fullString, std::string const &ending);

}


#endif //SCALER_FILETOOL_H
