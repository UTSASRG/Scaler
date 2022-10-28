#ifndef SCALER_RECTUPLE_H
#define SCALER_RECTUPLE_H

/**
 * This struct is the format that we record time and save to disk.
 */
struct RecTuple {
    uint64_t totalClockCycles; //8
    int64_t count; //8
    int32_t gap; //4
    float meanClockTick; //4
    int32_t durThreshold; //4
    uint32_t flags; //4
    uint8_t magicNum = 167; //1 Used to ensure the collected data format is recognized in python scripts.
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
};


#endif //SCALER_RECTUPLE_H
