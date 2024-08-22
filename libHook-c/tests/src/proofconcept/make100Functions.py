'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: Steven (Jiaxun) Tang <jtang@umass.edu>
'''
srcFile = open("milionFuncLibrary.cpp", "w")
headerFile = open("includes/milionFuncLibrary.h", "w")
invokeFile = open("invoke.cpp", "w")

headerFile.write('extern "C" {\n\n')

for i in range(10000):
    headerFile.write('void func'+str(i)+'();\n\n')

headerFile.write('}\n\n')
headerFile.close()


srcFile.write('#include <cstdio> \n\n')
srcFile.write('extern "C" {\n\n')

for i in range(10000):
    srcFile.write('void func'+str(i)+'(){\n\n')
    srcFile.write('printf("This is Function '+str(i)+'\\n");\n\n')
    srcFile.write('}\n\n')

srcFile.write('}\n\n')
srcFile.close()

for i in range(10000):
    invokeFile.write('func'+str(i)+'();\n')
invokeFile.close()
