A = [1, 2, 3, 4, 4, 5]


def binSearch(A, tgt):
    """
    Return lower bound of the segment array
    :param A: Array
    :param tgt: Target
    :return: Lower bound of segments in the segment array
    """
    lo = 0
    hi = len(A)
    md = 0
    while lo < hi:
        md = lo + (hi - lo) // 2
        if A[md] < tgt:
            lo = md + 1
        elif A[md] > tgt:
            hi = md
        elif A[md] == tgt:
            hi = md

    return lo-1


testList = [0.5, 1.5, 2.5, 3.5, 4.5, 5.5]
expectedVal = [-1, 0, 1, 2, 4, 5]
for i in range(len(testList)):
    lo = binSearch(A, testList[i])
    print(testList[i], lo)
