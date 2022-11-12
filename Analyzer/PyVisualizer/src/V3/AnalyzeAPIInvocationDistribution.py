import os
import traceback
from matplotlib import cm
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

scalerDataFolder = '/media/umass/datasystem/steven/Downloads/Detailed_Timing/blackscholes'

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
Coverage Report
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

'''
Counting histogram Report
'''
histogramRoot = os.path.join(scalerDataFolder, 'InvocationCountHist')
if not os.path.exists(histogramRoot):
    os.mkdir(histogramRoot)

print()

# create 3 data sets with 1,000 samples
mu, sigma = 200, 25
x = mu + sigma * np.random.randn(1000, 3)

totalCountArr = None
for threadId in recInfo.threadIdList:
    curThreadRecArray = readTimingStruct(os.path.join(scalerDataFolder, 'threadTiming_%s.bin' % threadId))
    # curThreadInvokedRecArray = [rec for rec in curThreadRecArray if rec.count > 0]

    times = np.array([rec.count for rec in curThreadRecArray])
    if totalCountArr is None:
        totalCountArr = times
    else:
        totalCountArr = np.vstack([totalCountArr, times])

    plt.figure()
    plt.hist(times, range=(1, times.max()), edgecolor="black", bins=50)
    plt.xlabel('Invocation counts')
    plt.ylabel('API number')
    plt.title('Histogram of invocation counts for thread %s' % (str(threadId)))

    plt.savefig(os.path.join(histogramRoot, threadId + '.png'))
    plt.close()
    #print(threadId, np.max(times))

totalCountArr = totalCountArr.transpose()
# totalCountArr = totalCountArr[np.where(totalCountArr > 0)]
plt.figure()
print(totalCountArr.max())
plt.hist(totalCountArr, range=(1, totalCountArr.max()), bins=50, stacked=True)
plt.xlabel('Invocation counts')
plt.ylabel('API number')
plt.title('Histogram of invocation counts for all threads staked')
plt.savefig(os.path.join(histogramRoot, 'total.png'))
plt.close()
