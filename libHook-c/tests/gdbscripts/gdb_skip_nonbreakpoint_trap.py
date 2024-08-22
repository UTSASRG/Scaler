'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
    You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
'''
import gdb

import sys
sys.setrecursionlimit(999999)

def stopHandler(event):
	
	if(type(event)==gdb.BreakpointEvent):
		print ("event type: breakpoint stop")
	else: 
		gdb.execute("signal 5",from_tty=True)
			

gdb.events.stop.connect(stopHandler)
