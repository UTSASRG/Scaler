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

def event_handler(event):

    canContinue = True
    curFrame = gdb.newest_frame()
    rspValue = curFrame.read_register('rsp')




#gdb.events.stop.connect(event_handler)
