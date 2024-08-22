/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/

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
