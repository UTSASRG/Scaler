/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_SCALEREXCEPTION_H
#define SCALER_SCALEREXCEPTION_H

#include <exception>
#include <type/Object.hpp>
#include <string>
#include <iostream>
#include <util/tool/Logging.h>

#define fatalError(errMsg) \
    ERR_LOG(errMsg);        \
    exit(-1);

#define fatalErrorS(errFmt, ...) \
    ERR_LOGS(errFmt,__VA_ARGS__);            \
    exit(-1);

#endif
