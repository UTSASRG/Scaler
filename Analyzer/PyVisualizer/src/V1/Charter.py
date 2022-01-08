# By: John Diep
# This charter script draws pi charts based on function profiling data
# This can handle timing data (timestamps and durations/execution times) as well as sample data

import json
import pprint
import matplotlib.pyplot as plt
import numpy as np
from DataAggregator import getFuncSampleCnts, getFuncDuras, getLibSampleCnts, searchCallstack, getLastLibSampleCnts, getLibDuras, getTIDSampleCnts, getTIDDuras

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

# This is a class for doing work on the pi charts
# It contains a boolean attribute for if we are doing operations on timestamps/durations
# The rest are related to the data structure from data aggregator where I store the information needed to display the 3 pi charts
# The last attribute is related to a list of wedge lists, the index of the wedge list attribute corresponds to the figure numbers of the wedge handlers
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
            # print(self.funcDict)
        self.wedgeList = []
    #Sets up chart
    def setupChart(self):
        #Draw the pi charts for threads, thread specific library calls, thread-specific and library-specific function calls
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

    def setupChartNew(self):
        # Generate a pi chart using just the sample counts for the last library call
        self.fig, self.axs = plt.subplots(1, figsize=(10, 10))
        libraryDict = {}
        # Add up all of the sample counts for the last library calls
        for tid in self.tidData.keys():
            # print(tid)
            tempDict = getLastLibSampleCnts(self.tidData,tid)
            libraryDict.update(tempDict)
        # print(libraryDict)

        #Draw the pi chart
        self.axs.pie(libraryDict.values(), labels = libraryDict.keys(), autopct = '%1.1f%%')
        self.axs.set_title("Library Sample Counts Pi Chart")

        self.axs.axis('equal')

        self.axs.legend(libraryDict.keys())
    #Updates the chart based on what chart was clicked on and what wedge was clicked on
    def updateChart(self, figNum, wedgeLabel):
        # Check if the chart that was selected from was either the thread pi chart or the library pi chart
        if figNum == 0:
            #Clear the necessary charts for drawing new ones
            self.axs[1].clear()
            self.axs[2].clear()
            self.tid = wedgeLabel
            #Use the timing data if self.timestamps is true
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
        #Selected from library pi chart
        elif figNum == 1:
            #Only need to update the function pi chart, thus the code below is the same as any other chart except it's changing the function pi chart
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

# A wedge hover handler is made for each individual chart, so in the case of this charter,
# I have a pi chart for showing the threads
# A pi chart for the libraries of a selected thread
# A pi chart for the functions of a selected library of the selected thread
# Thus 3 handlers will be initialized
class wedgeHoverHandler:
    def __init__(self, chart, wedges, figNum):
        # Set attributes as a pi chart instance, figure number, and a pi chart's wedge list
        self.chart = chart
        self.figNum = figNum
        self.wedges = wedges
        # Save the figure and axes we're drawing on to allow for the handler to update as needed
        self.fig = self.wedges[0].figure
        self.ax = self.wedges[0].axes
        #Set up an event handler
        self.motion = self.fig.canvas.mpl_connect('motion_notify_event', self.hover)
        if self.figNum < 2:
            self.click = self.fig.canvas.mpl_connect('button_press_event', self.onclick)

    #Function for exploding wedge on hover
    def hover(self, event):
        # If the wedges were somehow changed then reset them
        if self.wedges != self.chart.wedgeList[self.figNum][0]:
            self.wedges = self.chart.wedgeList[self.figNum][0]
        #If the hover is in a chart's axis
        if event.inaxes == self.ax:
            # print("fig num: ", self.figNum)
            #Iterate through the current figure's wedges and check if the hover occurred on a wedge
            for wedge in self.wedges:
                hit, _ = wedge.contains(event)
                # If hover is on wedge then explode it
                if hit:
                    theta = np.radians((wedge.theta1 + wedge.theta2) / 2)
                    wedge.set_center((.1 * np.cos(theta), .1 * np.sin(theta)))
                    self.fig.canvas.draw_idle()
                # Otherwise reset the wedges if they were previously exploded
                else:
                    wedge.set_center((0, 0))
                    self.fig.canvas.draw_idle()

    #Function for when a click event happens
    #Allows for selecting specific wedges and their respective libraries and function pi charts
    def onclick(self, event):
        #Check if user clicked on the top most or middle figure, if clicked on last figure then do nothing
        if self.figNum < 2:
            # Check if user clicked within the boundary of the charts
            if event.inaxes == self.ax:
                # Iterate through the individual wedges and check if the user clicked on a particular wedge
                for wedge in self.wedges:
                    # hit contains a boolean, it is true if user clicked on the current wedge, otherwise false
                    hit, _ = wedge.contains(event)
                    # hit was true then update the charts
                    if hit:
                        # self.fig.clf()
                        self.chart.updateChart(self.figNum, wedge.get_label())
                        self.fig.canvas.draw_idle()
                        break
                    # If hit was not true then reset the wedges of this chart if they were somehow changed
                    else:
                        if self.wedges != self.chart.wedgeList[self.figNum][0]:
                            self.wedges = self.chart.wedgeList[self.figNum][0]
                        #self.chart.setupChart()
                        #self.fig.canvas.draw_idle()
    #For safely removing the event handlers at the end of execution
    def disconnectHandlers(self):
        self.fig.canvas.mpl_disconnect(self.motion)
        if (self.figNum) < 2:
            self.fig.canvas.mpl_disconnect(self.click)

def main():
    #Get the data from the json file
    with open("perfMemcachedData.json", 'r') as j_file:
        tidData = json.load(j_file)
    #with open("fakeData.json", 'r') as fake_file:
    #    tidData = json.load(fake_file)
    #    fakeData = json.load(fake_file)
    # pprint.pprint(fakeData)
    # pprint.pprint(tidData[list(tidData.keys())[0]])
    # print(type(tidData))

    new = False

    # Initialize our pi charts
    pichart = charter(tidData, False)
    # pichart.setupChart()
    pichart.setupChartNew()
    # pichart.displayTotalSamples()
    if new:
        # Figure numbers for each event handler
        num1, num2, num3 = 0, 1, 2
        # Install Event handlers to allow for interactivity
        handler = wedgeHoverHandler(pichart, pichart.wedgeList[num1][0], num1)
        handler2 = wedgeHoverHandler(pichart, pichart.wedgeList[num2][0], num2)
        handler3 = wedgeHoverHandler(pichart, pichart.wedgeList[num3][0], num3)

        #Display our charts
        plt.show()
        #Disconnect all of our event handlers
        handler.disconnectHandlers()
        handler2.disconnectHandlers()
        handler3.disconnectHandlers()
    else:
        plt.show()
    return
main()