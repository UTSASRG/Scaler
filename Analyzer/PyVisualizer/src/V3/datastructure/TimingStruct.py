from collections import defaultdict
from ctypes import *


class VPG:
    def __init__(self):
        self.value = 0
        self.parentPercent = 0.0
        self.globalPercent = 0.0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Parent:%.2f Global:%.2f' % (self.parentPercent, self.globalPercent)


class VG:
    def __init__(self):
        self.value = 0
        self.globalPercent = 0.0

    def __str__(self):
        return 'Val:' + str(self.value) + ' Global:%.2f' % (self.globalPercent)


class VTG:
    def __init__(self):
        self.value = 0
        self.totalPercent = 0
        self.globalPercent = 0

    def __str__(self):
        return 'Val:' + str(self.value) + 'Total:%.2f Global:%.2f' % (self.totalPercent, self.globalPercent)


class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.totalClockCycles = VPG()  # Key: symbolName Val: symbolTime
        self.counts = VPG()

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.symbolName, self.counts.value, str(self.totalClockCycles.value))


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.totalClockCycles = VG()  # Key: symbolName Val: symbolTime
        self.counts = VG()
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.fileName, self.counts.value, str(self.totalClockCycles.value))


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.selfClockCycles = VTG()  # Key: symbolName Val: symbolTime
        self.childrenClockCycles = VTG()  # Key: symbolName Val: symbolTime
        self.extFileTiming = defaultdict(ExtFileRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return str(self.fileName)


class RecTuple(Structure):
    _fields_ = [
        ('totalClockCycles', c_uint64),
        ('count', c_int64),
        ('_gap', c_uint32),
        ('_meanClockTick', c_float),
        ('_durThreshold', c_uint32),
        ('_flags', c_uint32)]


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
