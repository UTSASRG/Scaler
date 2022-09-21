from collections import defaultdict
from ctypes import *


class VPG:
    def __init__(self):
        self.value = 0
        self.parentPercent = 0.0
        self.globalPercent = 0.0

    def __str__(self):
        return str(self.value)


class VG:
    def __init__(self):
        self.value = 0
        self.globalPercent = 0.0

    def __str__(self):
        return str(self.value)


class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.clockCycles = VPG()  # Key: symbolName Val: symbolTime
        self.counts = VPG()
        self.skipped: bool = False

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.symbolName, self.counts.value, str(self.clockCycles.value))


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.clockCycles = VPG()  # Key: symbolName Val: symbolTime
        self.counts = VPG()
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return 'Name: %s Count: %s Time: %s' % (self.fileName, self.counts.value, str(self.clockCycles.value))


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.clockCycles = VPG()  # Key: symbolName Val: symbolTime
        self.counts = VPG()
        self.extFileTiming = defaultdict(ExtFileRecord)  # Key: fileName Val: ExtSymTimingRecord

    def __str__(self):
        return str(self.value)


class RecTuple(Structure):
    _fields_ = [
        ('totalClockCycles', c_uint64),
        ('count', c_int64),
        ('gap', c_uint32),
        ('meanClockTick', c_float),
        ('durThreshold', c_uint32),
        ('flags', c_uint32)]
