'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
'''
import numpy as np
import pandas
import os
import matplotlib.pyplot as plt

DATA_PATH = '/media/umass/datasystem/steven/Downloads/SimpleApps'

# Test the std of innerTime. This is to verify the integrity of
print('Inner time')
for folder in os.listdir(DATA_PATH):
    print(folder, end='\t')
    for i in range(1, 9):
        # print(os.path.join(DATA_PATH, folder, '%d.csv' % (i)))
        df = pandas.read_csv(os.path.join(DATA_PATH, folder, '%d.csv' % (i)))
        # print(df.describe(include='all'))
        print(np.std(df['innerTime']), end='\t')
    print()

print('Outer time - Inner time')
for folder in os.listdir(DATA_PATH):
    print(folder, end='\t')
    for i in range(1, 9):
        # print(os.path.join(DATA_PATH, folder, '%d.csv' % (i)))
        df = pandas.read_csv(os.path.join(DATA_PATH, folder, '%d.csv' % (i)))
        # print(df.describe(include='all'))
        print(np.std(df['innerTime'] - df[' outerTime']), end='\t')  # One extra space in outertime
    print()

df = pandas.read_csv(os.path.join(DATA_PATH, 'Prehook', '5.csv'))
plt.scatter(range(len(df['innerTime'])), df['innerTime'])
plt.title('Prehook 5.csv')
plt.show()
print(df.describe())
