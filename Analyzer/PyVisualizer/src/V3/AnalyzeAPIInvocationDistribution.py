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
from util.Parser.DetailedTimeOutputPrarser import parseSingleSymDetailedTiming
from util.Parser.TimeOutputPrarser import readSymbolFiles, aggregatePerThreadArray, readTimingStruct

scalerDataFolder = '/tmp/scalerdata_14568349363894428'

recInfo = readSymbolFiles(scalerDataFolder)

invokedAPIPercentage = []
invokedAPIs = []
totalAPIs = []
for threadId in recInfo.threadIdList:
    curThreadRecArray = readTimingStruct(os.path.join(scalerDataFolder, 'threadTiming_%s.bin' % threadId))
    curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]
    invokedAPIs.append(len(curThreadInvokedRecArray))
    totalAPIs.append(len(curThreadRecArray))

'''
Output Report
'''
print(' \t ', end='')
for threadId in recInfo.threadIdList:
    print(threadId, end='\t')
print()

print('Invoked %', end='\t')
for perc in np.array(invokedAPIs) / np.array(totalAPIs):
    print('%2.2f%%' % (perc * 100), end='\t')
print()

print('Invoked APIs', end='\t')
for invokedAPI in invokedAPIs:
    print(invokedAPI, end='\t')

print()
print('Total APIs', end='\t')
for totalAPI in totalAPIs:
    print(totalAPI, end='\t')

print()
