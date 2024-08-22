/*
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
*/
#ifndef SCALER_RECTUPLE_H
#define SCALER_RECTUPLE_H

/**
 * This struct is the format that we record time and save to disk.
 */
struct RecTuple {
    uint64_t totalClockCycles; //8
    int64_t count; //8
    int32_t gap; //4
};


/**
 * This struct stores the total size and element size of an array.
 * On disk, this struct is followed by array elements
 */
struct ArrayDescriptor {
    uint64_t arrayElemSize;
    uint64_t arraySize;
    uint8_t magicNum = 167;  //1 Used to ensure the collected data format is recognized in python scripts.
};

/**
 * This struct is the format that we record detailed timing and save to disk.
 */
typedef int64_t TIMING_TYPE;

struct DetailedTimingDescriptor {
    TIMING_TYPE timingSize;
};

struct ThreadCreatorInfo {
    uint64_t threadCreatorFileId;
    uint64_t threadExecutionCycles;
    uint8_t magicNum = 167;  //1 Used to ensure the collected data format is recognized in python scripts.
    char threadName[16];
    void* threadFunctionPtr=nullptr;
};


#endif //SCALER_RECTUPLE_H
