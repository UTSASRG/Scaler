'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: John Diep <Sharpsteel321@gmail.com>
@author: Steven (Jiaxun) Tang <jtang@umass.edu>
'''

import struct

with open('simpleProgram_0x7fa3b750de00.bin', 'rb') as f:
    type = struct.unpack('b', f.read(1))
    if (type[0] == 1):
        print('NORMAL_FUNC')
    else:
        assert False;
    fileID = struct.unpack('q', f.read(8))[0]
    print('fileID=',fileID)
    funcID = struct.unpack('q', f.read(8))[0]
    print('funcID=',funcID)
    startTimestamp = struct.unpack('q', f.read(8))[0]
    print('startTimestamp=',startTimestamp)
    endTimeStamp = struct.unpack('q', f.read(8))[0]
    print('endTimeStamp=',endTimeStamp)
    firstChildIndex = struct.unpack('q', f.read(8))[0]
    print('firstChildIndex=',firstChildIndex)
    childrenSize = struct.unpack('q', f.read(8))[0]
    print('childrenSize=',childrenSize)

