from collections import defaultdict
from ctypes import *

'''
Display related parameter:
'''


class VLG:
    def __init__(self):
        self.value = 0
        self.localPercent = 0.0
        self.globalPercent = 0.0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Local:%.5f Global:%.5f' % (self.localPercent, self.globalPercent)


class VG:
    def __init__(self):
        self.value = 0
        self.globalPercent = 0.0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Global:%.5f' % (self.globalPercent)


class VLG:
    def __init__(self):
        self.value = 0
        self.localPercent = 0
        self.globalPercent = 0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Local:%.5f Global:%.5f' % (self.localPercent, self.globalPercent)


class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.totalClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.counts = VLG()

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.symbolName, 'self.counts', str(self.totalClockCycles))


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.totalClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.counts = VLG()
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.fileName, 'self.counts', str(self.totalClockCycles))


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.selfClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.childrenClockCycles = VLG()  # Key: symbolName Val: symbolTime
        self.extFileTiming = defaultdict(ExtFileRecord)  # Key: fileName Val: ExtSymTimingRecord

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
        ('totalClockCyclesUnScaled', c_uint64),
        ('count', c_int64),
        ('_gap', c_uint32),
        ('_meanClockTick', c_float),
        ('_flags', c_uint32)]


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
        ('_magicNum', c_uint8)]
