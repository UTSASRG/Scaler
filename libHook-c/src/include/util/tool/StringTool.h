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

    /**
     * Replace multiple space by one
     * @param butterOutput This buffer should have the same size as oriString.size
     * @return true->success false->fail
     */
    bool collapseStrSpace(const std::string &oriString,std::string& outString);

}


#endif //SCALER_FILETOOL_H
