#include <util/tool/StringTool.h>

bool scaler::strEndsWith(const std::string &fullString, const std::string &ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}


bool scaler::strStartsWith(const std::string &fullString, const std::string &starting) {
    if (fullString.length() >= starting.length()) {
        return (0 == fullString.compare(0, starting.length(), starting));
    } else {
        return false;
    }
}


bool scaler::strContains(const std::string &fullString, const std::string &keyword) {
    return fullString.find(keyword) != std::string::npos;
}

bool scaler::collapseStrSpace(const std::string &oriString, std::string &outString) {
    //todo: expensive op. Allocate with heap.
    outString = oriString;
    std::stringstream ss;

    bool spaceInserted = true;
    for (int i = 0; i < outString.size(); ++i) {
        auto &curChar = outString[i];
        if (outString[i] != ' ') {
            spaceInserted = false;
            ss << curChar;
        } else if (!spaceInserted) {
            ss << " ";
            spaceInserted = true;
        }
    }
    outString=ss.str();

    return true;
}

