#include <util/tool/StringTool.h>

bool scaler::strEndsWith(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool scaler::strContains(const std::string &fullString, const std::string &keyword) {
    return fullString.find(keyword) != std::string::npos;
}

