/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#include <util/tool/StringTool.h>
namespace scaler {
    bool strEndsWith(const std::string &fullString, const std::string &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }


    bool strStartsWith(const std::string &fullString, const std::string &starting) {
        if (fullString.length() >= starting.length()) {
            return (0 == fullString.compare(0, starting.length(), starting));
        } else {
            return false;
        }
    }


    bool strContains(const std::string &fullString, const std::string &keyword) {
        return fullString.find(keyword) != std::string::npos;
    }

    bool collapseStrSpace(const std::string &oriString, std::string &outString) {
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
        outString = ss.str();

        return true;
    }
}
