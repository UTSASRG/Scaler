//
// Created by st on 1/10/21.
//

#ifndef SCALER_FILETOOL_H
#define SCALER_FILETOOL_H


#include <cstddef>
#include <cstdio>
#include <vector>
#include <sstream>

namespace scaler {

    bool strEndsWith(std::string const &fullString, std::string const &ending);

    bool strContains(std::string const &fullString, std::string const &ending);

}


#endif //SCALER_FILETOOL_H
