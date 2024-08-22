'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
from collections import defaultdict
from ctypes import *
from typing import List, Any

'''
Display related parameter:
'''


class VLG:
    def __init__(self):
        self.value = 0
        self.localPercent = 0.0
        self.localRank = 0
        self.globalPercent = 0.0
        self.globalRank = 0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Local:%.5f Global:%.5f' % (self.localPercent, self.globalPercent)


class VG:
    def __init__(self):
        self.value = 0
        self.globalPercent = 0.0
        self.globalRank = 0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Global:%.5f' % (self.globalPercent)



class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.totalClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.counts = VLG()

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.symbolName, self.counts, str(self.totalClockCycles))


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.totalClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.counts = VLG()
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.fileName, self.counts, str(self.totalClockCycles))


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.selfClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.childrenClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.extFileTiming = defaultdict(ExtFileRecord)  # Key: fileName Val: ExtSymTimingRecord

        self.waitClockCycles = VLG()
        self.waitCount=VLG()
        self.joinClockCycles=VLG()
        self.joinCount=VLG()
    def __str__(self):
        return str(self.fileName)


class RecordingInfo:
    '''
    Used to share common information needed in processing between different functions
    '''

    def __init__(self):
        self.fileNameList = []
        self.pthreadFileId = None
        self.threadIdList = []
        self.realFileIdList = []
        self.symbolNameList = []
        self.symbolFileIdList = []
        self.symIdInFileList = []


'''
C data structures

These structs should be consistent with https://github.com/UTSASRG/Scaler/blob/feature-tuneParm/libHook-c/src/include/type/RecTuple.h
'''


class RecTuple(Structure):
    _fields_ = [
        ('totalClockCycles', c_uint64),
        ('count', c_int64),
        ('_gap', c_uint32)
    ]


class ArrayDescriptor(Structure):
    _fields_ = [
        ('arrayElemSize', c_uint64),
        ('arraySize', c_uint64),
        ('_magicNum', c_uint8)]


class DetailedTimingDescriptor(Structure):
    _fields_ = [
        ('timingSize', c_int64)]


class ThreadCreatorInfo(Structure):
    _fields_ = [
        ('threadCreatorFileId', c_int64),
        ('threadExecutionCycles', c_int64),
        ('_magicNum', c_uint8),
        ('threadName',c_char*16),
        ('threadFunctionPtr',c_int64)

    ]
