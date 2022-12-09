import numpy as np


class Metric:
    def __init__(self):
        self.metricList = []

    def append(self, *args, **kwargs):
        self.metricList.append(*args, **kwargs)

    def std(self):
        return np.std(np.array(self.metricList))

    def mean(self):
        return np.mean(np.array(self.metricList))

