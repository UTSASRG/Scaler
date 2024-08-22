'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
'''
# The following algorithms are used in ProcinfoParser to find the left bound in an array representing the starting element of the array
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
    assert (lo == expectedVal[i])
