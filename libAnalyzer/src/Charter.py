# By: John Diep

import json
import pprint
import matplotlib.pyplot as plt
import numpy as np

# Functions for getting function information, library or thread information
def getFuncSampleCnts(aDict, TID, Lib):
    funcDict = {}
    for libs in aDict[TID][0]:
        for library, val in libs.items():
            if library == Lib:
                for func, funcVals in val[0].items():
                    if func in funcDict.keys():
                        funcDict[func] += funcVals[0]
                    else:
                        funcDict[func] = funcVals[0]
    return funcDict

def getFuncDuras(aDict, TID, Lib):
    funcDict = {}
    for libs in aDict[TID][0]:
        for library, val in libs.items():
            if library == Lib:
                for func, funcVals in val[0].items():
                    # print(funcVals)
                    if func in funcDict.keys():
                        funcDict[func] += funcVals[1][2]
                    else:
                        funcDict[func] = funcVals[1][2]
    return funcDict

def getLibSampleCnts(aDict, TID):
    libDict = {}
    for libs in aDict[TID][0]:
        # print(libs)
        for library, val in libs.items():
            if library in libDict.keys():
                libDict[library] += val[1]
            else:
                libDict[library] = val[1]
    return libDict

def getLibDuras(aDict, TID):
    libDict = {}
    for libs in aDict[TID][0]:
        # print(libs)
        for library, val in libs.items():
            if library in libDict.keys():
                libDict[library] += val[2]
            else:
                libDict[library] = val[2]
    return libDict

def getTIDSampleCnts(aDict):
    aList = []
    for val in aDict.values():
        #aList.insert(0, val[1])
        aList.append(val[1])
    return aList

def getTIDDuras(aDict):
    aList = []
    for val in aDict.values():
        #aList.insert(0, val[1])
        aList.append(val[2])
    return aList

def overlapCalc():
    pass
    '''
    Handles the problem of threads in parallel
    
    Need to weigh the impact of threads differently in comparison to other threads in parallel
    
    Things to consider:
    Thread start and end time
    Thread Duration
    Thread relevance
    '''


class charter:
    def __init__(self,tidDict, timestamps=False):
        self.timestamps = timestamps
        self.tidData = tidDict
        self.tid = list(self.tidData)[0]
        if timestamps:
            self.librDict = getLibDuras(self.tidData, self.tid)
            self.library = list(self.librDict)[0]
            self.funcDict = getFuncDuras(self.tidData, self.tid, self.library)
        else:
            self.librDict = getLibSampleCnts(self.tidData, self.tid)
            self.library = list(self.librDict)[0]
            self.funcDict = getFuncSampleCnts(self.tidData, self.tid, self.library)
        self.wedgeList = []
    #Sets up chart
    def setupChart(self):
        self.fig, self.axs = plt.subplots(3, figsize=(10, 10))
        if self.timestamps:
            self.wedgeList.append(self.axs[0].pie(getTIDDuras(self.tidData), labels=self.tidData.keys(), autopct='%1.1f%%'))
        else:
            self.wedgeList.append(self.axs[0].pie(getTIDSampleCnts(self.tidData), labels=self.tidData.keys(), autopct='%1.1f%%'))
        self.axs[0].set_title("Thread Pi Chart")

        self.wedgeList.append(self.axs[1].pie(self.librDict.values(), labels=self.librDict.keys(), autopct='%1.1f%%'))
        self.axs[1].set_title(self.tid + " Libraries Pi Chart")

        self.wedgeList.append(self.axs[2].pie(self.funcDict.values(), labels=self.funcDict.keys(), autopct='%1.1f%%'))
        self.axs[2].set_title(self.library + " Functions Pi Chart")

        self.axs[0].axis('equal')
        self.axs[1].axis('equal')
        self.axs[2].axis('equal')

        self.axs[0].legend(self.tidData.keys())
        self.axs[1].legend(self.librDict.keys())
        self.axs[2].legend(self.funcDict.keys())

    #Updates the chart based on what chart was clicked on and what wedge was clicked on
    def updateChart(self, figNum, wedgeLabel):
        if figNum == 0:
            self.axs[1].clear()
            self.axs[2].clear()
            self.tid = wedgeLabel
            if self.timestamps:
                self.librDict = getLibDuras(self.tidData, self.tid)
                self.library = list(self.librDict)[0]
                self.funcDict = getFuncDuras(self.tidData, self.tid, self.library)
            else:
                self.librDict = getLibSampleCnts(self.tidData, self.tid)
                self.library = list(self.librDict)[0]
                self.funcDict = getFuncSampleCnts(self.tidData, self.tid, self.library)

            self.wedgeList[1] = self.axs[1].pie(self.librDict.values(), labels=self.librDict.keys(), autopct='%1.1f%%')
            self.axs[1].set_title(self.tid + " Libraries Pi Chart")

            self.wedgeList[2] = self.axs[2].pie(self.funcDict.values(), labels=self.funcDict.keys(), autopct='%1.1f%%')
            self.axs[2].set_title(self.library + " Functions Pi Chart")

            self.axs[1].axis('equal')
            self.axs[2].axis('equal')

            self.axs[1].legend()
            self.axs[2].legend()
        elif figNum == 1:
            self.axs[2].clear()
            self.library = wedgeLabel
            if self.timestamps:
                self.funcDict = getFuncDuras(self.tidData, self.tid, self.library)
            else:
                self.funcDict = getFuncSampleCnts(self.tidData, self.tid, self.library)
            # print(self.library)
            # print(self.funcDict)
            self.wedgeList[2] = self.axs[2].pie(self.funcDict.values(), labels=self.funcDict.keys(), autopct='%1.1f%%')
            self.axs[2].set_title(self.library + " Functions Pi Chart")

            self.axs[2].axis('equal')
            self.axs[2].legend()
    def displayTotalSamples(self):
        total = 0
        for vals in self.tidData.values():
            total += vals[1]
        print("Total Samples: " + str(total) + "Total time: " + str(total * 1/997))
    def threadExecution(self):
        total = 0


class wedgeHoverHandler:
    def __init__(self, chart, wedges, figNum):
        self.chart = chart
        self.figNum = figNum
        self.wedges = wedges
        self.fig = self.wedges[0].figure
        self.ax = self.wedges[0].axes
        self.motion = self.fig.canvas.mpl_connect('motion_notify_event', self.hover)
        if self.figNum < 2:
            self.click = self.fig.canvas.mpl_connect('button_press_event', self.onclick)

    #Function for exploding wedge on hover
    def hover(self, event):
        if self.wedges != self.chart.wedgeList[self.figNum][0]:
            self.wedges = self.chart.wedgeList[self.figNum][0]
        if event.inaxes == self.ax:
            # print("fig num: ", self.figNum)
            for wedge in self.wedges:
                hit, _ = wedge.contains(event)
                if hit:
                    theta = np.radians((wedge.theta1 + wedge.theta2) / 2)
                    wedge.set_center((.1 * np.cos(theta), .1 * np.sin(theta)))
                    self.fig.canvas.draw_idle()
                else:
                    wedge.set_center((0, 0))
                    self.fig.canvas.draw_idle()

    #Function for when a click event happens
    #Allows for selecting specific wedges and their respective libraries and function pi charts
    def onclick(self, event):
        if self.figNum < 2:
            if event.inaxes == self.ax:
                for wedge in self.wedges:
                    hit, _ = wedge.contains(event)
                    if hit:
                        # self.fig.clf()
                        self.chart.updateChart(self.figNum, wedge.get_label())
                        self.fig.canvas.draw_idle()
                    else:
                        if self.wedges != self.chart.wedgeList[self.figNum][0]:
                            self.wedges = self.chart.wedgeList[self.figNum][0]
                        #self.chart.setupChart()
                        #self.fig.canvas.draw_idle()

    def disconnectHandlers(self):
        self.fig.canvas.mpl_disconnect(self.motion)
        if (self.figNum) < 2:
            self.fig.canvas.mpl_disconnect(self.click)

def main():
    #Get the data from the json file
    # with open("perfMemcachedData.json", 'r') as j_file:
    #    tidData = json.load(j_file)
    with open("fakeData.json", 'r') as fake_file:
        tidData = json.load(fake_file)
    #    fakeData = json.load(fake_file)
    # pprint.pprint(fakeData)
    # pprint.pprint(tidData)
    # print(type(tidData))

    # Initialize our pi charts
    pichart = charter(tidData, False)
    pichart.setupChart()
    pichart.displayTotalSamples()
    # Figure numbers for each event handler
    num1, num2, num3 = 0, 1, 2
    #Install Event handlers to allow for interactivity
    handler = wedgeHoverHandler(pichart, pichart.wedgeList[num1][0], num1)
    handler2 = wedgeHoverHandler(pichart, pichart.wedgeList[num2][0], num2)
    handler3 = wedgeHoverHandler(pichart, pichart.wedgeList[num3][0], num3)

    #Display our charts
    plt.show()
    #Disconnect all of our event handlers
    handler.disconnectHandlers()
    handler2.disconnectHandlers()
    handler3.disconnectHandlers()
    return
main()