srcFile = open("test/milionFuncLibrary.cpp", "w")
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
