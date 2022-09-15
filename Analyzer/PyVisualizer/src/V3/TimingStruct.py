from collections import defaultdict
from ctypes import *


class VPG:
    def __init__(self):
        self.value = None
        self.parentPercent = None
        self.globalPercent = None


class VG:
    def __init__(self):
        self.value = None
        self.globalPercent = None


class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.clockCycles: VPG = None  # Key: symbolName Val: symbolTime
        self.counts: VPG = None
        self.skipped: bool = False


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.clockCycles: VPG = None  # Key: symbolName Val: symbolTime
        self.counts: VPG = None
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.clockCycles: VPG = None  # Key: symbolName Val: symbolTime
        self.counts: VPG = None
        self.extFileTiming = defaultdict(ExtFileRecord)  # Key: fileName Val: ExtSymTimingRecord


class RecTuple(Structure):
    _fields_ = [
        ('totalClockCycles', c_uint64),
        ('count', c_int64),
        ('gap', c_uint32),
        ('meanClockTick', c_float),
        ('durThreshold', c_uint32),
        ('flags', c_uint32)]
