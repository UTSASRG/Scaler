import os
import traceback

import matplotlib.pyplot as plt
import pandas as pd
import struct
import numpy as np
from multiprocessing import Pool
from multiprocessing import Pool, cpu_count
import time

from datastructure.TimingStruct import ArrayDescriptor


def parseSingleSymDetailedTiming(ROOT_PATH: str, threadId: str, tgtSymIds: list):
    # hasPoints = False
    if tgtSymIds:
        tgtSymIds = sorted(tgtSymIds)

    rlt = {}  # SymId, timingArray
    with open(os.path.join(ROOT_PATH, 'threadDetailedTiming_%s.bin' % (threadId)), 'rb') as f:
        # Get the amount of symbols in this file
        arrayDesc = ArrayDescriptor()
        f.readinto(arrayDesc)
        assert (arrayDesc.arrayElemSize == 0)
        assert (arrayDesc._magicNum == 167)

        if tgtSymIds is None:
            tgtSymIds = range(arrayDesc.arraySize)
        assert (tgtSymIds[-1] < arrayDesc.arraySize)

        detailedTimingForCurSym = None
        for curSymId in range(arrayDesc.arraySize):
            symDetailedTimingDesc = ArrayDescriptor()
            f.readinto(symDetailedTimingDesc)

            assert (symDetailedTimingDesc.arrayElemSize == 8)
            assert (symDetailedTimingDesc._magicNum == 167)
            if curSymId < tgtSymIds[0]:
                # Only read specified symbol
                f.seek(symDetailedTimingDesc.arraySize * symDetailedTimingDesc.arrayElemSize, os.SEEK_CUR)
                continue
            elif curSymId == tgtSymIds[0]:
                detailedTimingForCurSym = np.array(struct.unpack_from('<%dQ' % (symDetailedTimingDesc.arraySize),
                                                                      f.read(symDetailedTimingDesc.arraySize *
                                                                             symDetailedTimingDesc.arrayElemSize)))
                rlt[curSymId] = detailedTimingForCurSym
                tgtSymIds = tgtSymIds[1:]
                if len(tgtSymIds) == 0:
                    break
            else:
                # There are duplicate or negative values inside tgtSymIds
                assert (False)
    # assert (len(tgtSymIds) == 0)
    return rlt
