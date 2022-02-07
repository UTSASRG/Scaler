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

