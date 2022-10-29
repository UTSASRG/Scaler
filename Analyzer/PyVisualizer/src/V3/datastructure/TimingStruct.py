from collections import defaultdict
from ctypes import *

'''
Display related parameter:
'''


class VPG:
    def __init__(self):
        self.value = 0
        self.parentPercent = 0.0
        self.globalPercent = 0.0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Parent:%.5f Global:%.5f' % (self.parentPercent, self.globalPercent)


class VG:
    def __init__(self):
        self.value = 0
        self.globalPercent = 0.0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Global:%.5f' % (self.globalPercent)


class VTG:
    def __init__(self):
        self.value = 0
        self.totalPercent = 0
        self.globalPercent = 0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Total:%.5f Global:%.5f' % (self.totalPercent, self.globalPercent)


class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.totalClockCycles = VPG()  # Key: symbolName Val: symbolTime
        self.counts = VPG()

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.symbolName, 'self.counts', str(self.totalClockCycles))


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.totalClockCycles = VPG()  # Key: symbolName Val: symbolTime
        self.counts = VPG()
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.fileName, 'self.counts', str(self.totalClockCycles))


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.selfClockCycles = VTG()  # Key: symbolName Val: symbolTime
        self.childrenClockCycles = VTG()  # Key: symbolName Val: symbolTime
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
        ('count', c_int64),
        ('_gap', c_uint32),
        ('_meanClockTick', c_float),
        ('_durThreshold', c_uint32),
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
