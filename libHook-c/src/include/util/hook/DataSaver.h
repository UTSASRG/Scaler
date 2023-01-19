#ifndef SCALER_DATASAVER_H
#define SCALER_DATASAVER_H

#include<util/hook/HookContext.h>


class DataSaver {
public:
    char initializeMe = 0;

    ~DataSaver();
};

extern thread_local DataSaver saverElem;


//#define INSTR_TIMING
#ifdef INSTR_TIMING
extern const int TIMING_REC_COUNT;
extern __thread TIMING_TYPE **detailedTimingVectors;
extern __thread TIMING_TYPE *detailedTimingVectorSize;
#endif

extern bool dataSaveLock;




void saveData(HookContext *context, bool finalize = false);

#endif
