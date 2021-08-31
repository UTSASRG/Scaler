import gdb

import sys
sys.setrecursionlimit(9000)

def stopHandler(event):
	
	if(type(event)==gdb.BreakpointEvent):
		print ("event type: breakpoint stop")
	else: 
		print(event)
		try:
			gdb.execute("signal 5",from_tty=True)
		except:
			

gdb.events.stop.connect(stopHandler)
