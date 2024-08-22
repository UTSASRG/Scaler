/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef EXTSYMINFO_H
#define EXTSYMINFO_H

#include <string>

namespace scaler {

    typedef ssize_t FileID;
    typedef ssize_t SymID;
    typedef ssize_t FuncID;

    enum SymbolType : int8_t {
        NULL_API = 0,
        C_PLT_API = 1,
        C_DYN_API = 2,
        C_DL_API = 3,
        PY_API = 4
    };


    /**
    * Symbol information
    */
    struct ExtSymInfo {
        int64_t fileId = -1;//(8 bytes) Stores fileID for this symbol
        int64_t symIdInFile = -1; //(8 bytes) todo: change the definition of symbol id so we only need to save one id. (Put all symbols in one list, one simple id corresponds to one symbol in a file. Same simple in different file is considered as different)
        int64_t realFileId=-1; //(8 bytes) Stores the origin file that holds this symbol
        uint64_t pltStubId = 0; //(8 bytes)
        int32_t initialGap = 0;//8 Bytes. Initial gap value
        uint8_t **realAddrPtr = nullptr; //Used for  Stores a pointer to real address. This address is stored as a pointer to pointer because the real address pointer cannot change.
        uint8_t *dlsymRealAddr = nullptr; //Dlsym loaded symbol do not have
        uint8_t *pltEntryAddr = nullptr; //Used for C_PLT_API
        uint8_t *pltSecEntryAddr = nullptr; //Used for C_PLT_API
        uint8_t** gotEntryAddr; //Used for C_PLT_API and C_DYN_API
        void* addressOverride = nullptr; //Used to store
        SymbolType symbolType = SymbolType::NULL_API;
    };
}
#endif