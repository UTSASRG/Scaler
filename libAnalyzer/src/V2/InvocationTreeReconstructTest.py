import json
import Parse_Scaler_Output as pso
if __name__ == "__main__":
    with open("C:/Users/John/PycharmProjects/Scaler/libAnalyzer/tests/Test_Cases/Scaler_Tests/simpleProgram.json", 'r') as test_file:
        testDict = json.load(test_file)
    for tid in testDict:
        InvoNode = pso.InvocationTree.reconstructFromDict(testDict[tid])
    pso.printTreeByLevel(InvoNode)
    pso.printTreeBranches(InvoNode)