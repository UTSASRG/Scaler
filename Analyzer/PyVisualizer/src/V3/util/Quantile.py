'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven(Jiaxun) Tang jtang@umass.edu
'''
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
