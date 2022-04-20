from collections import defaultdict


class ExtSymRecord:
    def __init__(self):
        self.symbolName = ''
        self.timePercent = 0  # With respect to library
        self.time = 0  # Key: symbolName Val: symbolTime


class ExtFileRecord:
    def __init__(self):
        self.fileName = ''
        self.totalExtTime = 0
        self.totalExtTimePercent = 0  # With respect to the parent's percentage
        self.extSymTiming = defaultdict(ExtSymRecord)  # Key: fileName Val: ExtSymTimingRecord


class FileRecord:
    def __init__(self):
        self.fileName = ''
        self.selfDuration = 0
        self.totalDuration = 0
        self.selfDurationPercent = 0  # With respect to total time
        self.extFileTiming = defaultdict(ExtFileRecord)  # Key: fileName Val: ExtSymTimingRecord
