'''
This file is part of Scaler.
Scaler is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Scaler is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

@author: John Diep <Sharpsteel321@gmail.com>
@author: Steven (Jiaxun) Tang <jtang@umass.edu>
'''
import json
import Parse_Scaler_Output as pso
if __name__ == "__main__":
    with open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/Test_Cases/Scaler_Tests/simpleProgram.json", 'r') as test_file:
        testDict = json.load(test_file)
    for tid in testDict:
        InvoNode = pso.InvocationTree.reconstructFromDict(testDict[tid])
    pso.printTreeByLevel(InvoNode)
    pso.printTreeBranches(InvoNode)