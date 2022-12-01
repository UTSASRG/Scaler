import numpy as np


def calcQuantile(array):
    min = np.min(array)
    q1 = np.quantile(array, 0.25)
    q2 = np.quantile(array, 0.5)
    q3 = np.quantile(array, 0.75)
    q4 = np.quantile(array, 1)
    iqr = q3 - q1
    qLower = q1 - 1.5 * iqr
    qUpper = q3 + 1.5 * iqr
    return min, q1, q2, q3, q4, iqr, qLower, qUpper
