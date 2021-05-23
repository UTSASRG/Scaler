import gdb
import uuid

gdb.Breakpoint('asmHookHandlerSec')


def getAllRegisterNames():
    curFrame = gdb.newest_frame()
    regnames = []
    for registerDesc in curFrame.architecture().registers():
        regnames.append(registerDesc.name)
    return regnames


prevRegValue = {}

asmHookHanlderAddr = None
asmHookHanlderRetAddr = None


def event_handler(event):
    global asmHookHanlderAddr
    global asmHookHanlderRetAddr
    global prevRegValue

    canContinue=True
    if type(event) == gdb.BreakpointEvent:
        curFrame = gdb.newest_frame()
        regNames = getAllRegisterNames()
        curBreakpointLoc = curFrame.read_register('rip')

        if asmHookHanlderAddr is None:
            asmHookHanlderAddr = curBreakpointLoc
            # Set breakpoint in the end
            gdb.execute('break *' + str(hex((int(curBreakpointLoc) + 394))))
        elif asmHookHanlderRetAddr is None:
            asmHookHanlderRetAddr = curBreakpointLoc

        if curBreakpointLoc == asmHookHanlderAddr:
            for regName in regNames:
                prevRegValue[regName] = curFrame.read_register(regName)
        elif curBreakpointLoc == asmHookHanlderRetAddr:
            for regName in regNames:
                if prevRegValue[regName] != curFrame.read_register(regName) and regName!='rip':
                    print('Register change', regName,prevRegValue[regName],curFrame.read_register(regName))
                    canContinue=False
            prevRegValue = {}
            asmHookHanlderAddr=None
            asmHookHanlderRetAddr=None

        # if canContinue:
        #     gdb.execute('c')

gdb.events.stop.connect(event_handler)
