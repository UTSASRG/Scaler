
/*
 * @file   recordentires.h
 * @brief  Managing record entry for each thread. Since each thread will have different entries,
 There is no need to use lock here at all.
 The basic idea of having pool is
 to reduce unnecessary memory allocation and deallocation operations, similar
 to slab manager of Linux system. However, it is different here.
 There is no memory deallocation for each pool.
 In the same epoch, we keep allocating from
 this pool and udpate correponding counter, updating to next one.
 When epoch ends, we reset the counter to 0 so that we can reuse all
 memory and there is no need to release the memory of recording entries.

 * @author Tongping Liu <http://www.cs.umass.edu/~tonyliu>
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.hh"
#include "mm.hh"
#include "xdefines.hh"
#include "recordentries.hh"
#include "xthread.hh"

//Entry* recordEntries::alloc() {
//    Entry* entry = NULL;
//	//	PRINF("allocEntry, _cur %ld\n", _cur);
//    if(_cur < _total) {
//      entry = (Entry*)&_start[_cur];
//      _cur++;
//    } else {
//      // There are no enough entries now; re-allocate new entries now.
//      PRWRN("Not enough entries, now _cur %lu, _total %lu at %p!!!\n", _cur, _total, &_cur);
//      xthread::invokeCommit();
//      entry = (Entry*)&_start[_cur];
//    }
//    return entry;
//}
//
