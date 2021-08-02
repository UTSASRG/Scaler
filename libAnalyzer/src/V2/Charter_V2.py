import json
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import numpy as np
import tkinter as tk
from tkinter import filedialog
import shutil

class charter:
    def __init__(self, chartData, figSuffix=""):
        """
        Initializes a charter instance

        The wedge list will store the return values of the pie chart created in setupChart()
        Therefore each element will be a tuple of:
        A matplotlib patches list
        A matplotlib texts list
        A matplotlib autotexts list

        The self.onThreadChart simply tells us when we are viewing the thread chart which was loaded in from
        the "Threads" data in chart_data.json
        :param chartData: Data loaded in from the chart_data.json
        :param figSuffix: A string which will be appended to a figure's file name, if we save one.
        """
        self.chartData = chartData
        self.wedgeList = []
        self.onThreadChart = True
        self.figSuffix = figSuffix

    # Sets up chart
    def setupChart(self, key):
        """
        This will set up the chart, one can select the data they want to display by passing in the appropriate
        key string.

        However, currently this method can only handle a normal dictionary.
        There is no adaptability implemented for nested dictionaries yet.

        :param key: A key from the chart_data json file. Ex: "Threads" or "funcs"
        :return: Nothing
        """
        # Draw the pi charts for threads, thread specific library calls, thread-specific and library-specific function calls
        self.fig, self.axs = plt.subplots(1, figsize=(10, 10))
        self.key = key

        print(self.chartData[key].keys())
        # self.wedgeList.append(self.axs.pie(self.chartData["tidLibs"]['perf-?/87902'].values(), labels=self.chartData["tidLibs"]['perf-?/87902'].keys(), autopct='%1.1f%%', normalize=True))
        # self.axs.set_title(f"perf-?/87902 Pie Chart")
        self.wedgeList.append(self.axs.pie(self.chartData[key].values(), labels=self.chartData[key].keys(), autopct='%1.1f%%', normalize=True))
        self.axs.set_title(f"{key} Pie Chart")
        # print(len(self.wedgeList[0]))
        self.axs.axis('equal')

        self.axs.legend(self.chartData[key].keys())


    def updateBack(self, event):
        """
        A back button method. This is for when the back button is clicked on.

        If we are on a library chart from tidlibs then we will go back to the thread chart when the back button
        is clicked on.

        If the back button is clicked on the thread chart, nothing will happen.

        :param event: Event data from the on click-handling
        :return: nothing
        """
        if not self.onThreadChart:
            self.onThreadChart = True
            self.axs.clear()

            self.wedgeList[0] = self.axs.pie(self.chartData["Threads"].values(), labels=self.chartData["Threads"].keys(), autopct='%1.1f%%', normalize=True)
            self.axs.set_title("Thread Pi Chart")

            self.axs.axis('equal')

            self.axs.legend(self.chartData["Threads"].keys())

            plt.draw()
        else:
            print("Did Nothing. Success.")
        return

    def updateChart(self, label):
        """
        Method to be called by the on_click event when a wedge is clicked on.
        Currently only works for tidlibs and threads.
        If the chart is displaying a chart of all of the threads then the user can click on a thread to
        view all of the libraries it called and the proportion of the thread's execution time they took up

        Currently, when a wedge is clicked on, the figure we switch to will also be saved if an output directory
        was selected.

        This will only change charts only if a thread wedge was clicked on. If a library wedge is clicked on,
        nothing will happen.
        :param label: A wedge label
        :return: Nothing
        """
        if self.onThreadChart:
            if directory:
                figName = f"{label.replace('-?/', '_')}" + self.figSuffix + ".png"
                #filepath = '/'.join(directory, figName)
            else:
                figName = ""
            self.onThreadChart = False
            self.axs.clear()

            self.wedgeList[0] = self.axs.pie(self.chartData["tidLibs"][label].values(), labels=self.chartData["tidLibs"][label].keys(), autopct='%1.1f%%', normalize=True)
            self.axs.set_title(f"{label} Library Totals")

            self.axs.axis('equal')

            self.axs.legend(self.chartData["tidLibs"][label].keys())
            if figName:
                if figName not in saveFigList:
                    saveFigList.append(figName)
                    plt.savefig(figName, orientation='landscape')
                plt.draw()
        return
    # This is a method that will be called when the save button is clicked on.
    # If there was no directory selected then we will not save anything.
    def saveThreadLibFigs(self, event):
        """
        Method to be called when the save button is clicked.

        If a directory was selected, then we will save a figure of every thread's data stored in tidlibs.

        Each figure's file name will contain the tid in it.

        One may pass in a different suffix to the figSuffix parameter of the charter class.
        This will be tacked onto the end of the tid in the file name of the figures.
        It is helpful in giving quick descriptive information about the figure.
        :param event: Event data passed in by save button when clicked
        :return: Nothing
        """
        for tid in self.chartData[self.key]:
            if directory:
                figName = f"{tid.replace('-?/', '_')}" + self.figSuffix + ".png"
                # filepath = '/'.join(directory, figName)
            else:
                figName = ""
            self.onThreadChart = False
            self.axs.clear()

            self.wedgeList[0] = self.axs.pie(self.chartData["tidLibs"][tid].values(),
                                             labels=self.chartData["tidLibs"][tid].keys(), autopct='%1.1f%%',
                                             normalize=True)
            self.axs.set_title(f"{tid} Library Totals")

            self.axs.axis('equal')

            self.axs.legend(self.chartData["tidLibs"][tid].keys())
            if figName:
                if figName not in saveFigList:
                    saveFigList.append(figName)
                    plt.savefig(figName, orientation='landscape')
                # plt.draw()
        plt.close()
        return


class wedgeHoverHandler:
    """
    A class for dealing with a chart's wedge hover and click functionality.
    A wedge hover handler is made for each individual chart
    """
    def __init__(self, chart, wedges):
        """

        :param chart: A charter instance
        :param wedges: A matplotlib pie chart patches list, each element represents a wedge in the pie chart
        """
        # Set attributes as a pi chart instance, figure number, and a pi chart's wedge list
        self.chart = chart
        self.wedges = wedges
        # Save the figure and axes we're drawing on to allow for the handler to update as needed
        self.fig = self.wedges[0].figure
        self.ax = self.wedges[0].axes
        #Set up an event handler
        self.motion = self.fig.canvas.mpl_connect('motion_notify_event', self.hover)
        self.click = self.fig.canvas.mpl_connect('button_press_event', self.onclick)

    def hover(self, event):
        """
        Hover event handler
        Matplotlib will call this function when a motion_notify_event is detected.

        We will explode the wedge we are hovering on to show the user which wedge they are looking at.

        Otherwise we will try to reset the chart, if the user is not hovering on a wedge
        :param event: Event data from matplotlib
        :return: Nothing
        """
        # If the wedges were somehow changed then reset them
        if self.wedges != self.chart.wedgeList[0][0]:
            self.wedges = self.chart.wedgeList[0][0]
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
        """
        For handling an on_click event.

        Currently this on click handler will deal with the thread -> library chart change.

        So when we are currently displaying the "Threads" data and the user clicks on a thread's wedge.

        The chart will update to show all of the libraries associated with that thread which is stored in "tidlibs"

        If a wedge is clicked on, we will change to a new chart based on the label of that wedge.
        :param event: matplotlib event data
        :return: nothing
        """
        if self.chart.onThreadChart:
            # Check if user clicked within the boundary of the charts
            if event.inaxes == self.ax:
                # Iterate through the individual wedges and check if the user clicked on a particular wedge
                for wedge in self.wedges:
                    # hit contains a boolean, it is true if user clicked on the current wedge, otherwise false
                    hit, _ = wedge.contains(event)
                    # hit was true then update the charts
                    if hit:
                        # self.fig.clf()
                        self.chart.updateChart(wedge.get_label())
                        self.fig.canvas.draw_idle()
                        break
                    # If hit was not true then reset the wedges of this chart if they were somehow changed
                    else:
                        if self.wedges != self.chart.wedgeList[0][0]:
                            self.wedges = self.chart.wedgeList[0][0]
                        #self.chart.setupChart()
                        #self.fig.canvas.draw_idle()

    def disconnectHandlers(self):
        """
        Safely disconnects any event handlers.
        :return: Nothing
        """
        self.fig.canvas.mpl_disconnect(self.motion)
        self.fig.canvas.mpl_disconnect(self.click)

def main():
    with open("chart_data.json", 'r') as j_file:
        chartData = json.load(j_file)

    Chart = charter(chartData, figSuffix="_perf_Fig_nokernel_noinlined")

    Chart.setupChart("libsScaled")

    # back = plt.axes([0.7, 0.05, 0.1, 0.075])
    # bback = Button(back, "Back", color='0.5', hovercolor='1.0')
    # bback.on_clicked(Chart.updateBack)

    '''save = plt.axes([0.7, 0.05, 0.1, 0.075])
    saveButton = Button(save, "Save", color='0.5', hovercolor='1.0')
    saveButton.on_clicked(Chart.saveThreadLibFigs)'''

    handler = wedgeHoverHandler(Chart, Chart.wedgeList[0][0])

    plt.show()

    handler.disconnectHandlers()

if __name__ == "__main__":

    '''root = tk.Tk()
    root.withdraw()
    print("Select a output directory for figures.")
    directory = filedialog.askdirectory()
    saveFigList = []'''

    main()
    # We created a bunch of figures in the directory that charter is in. Thus we will
    # move them all to the directory the user selected.
    '''for figName in saveFigList:
        shutil.move(figName, directory+'/'+figName)'''