import json
import matplotlib.pyplot as plt
import numpy as np

class charter:
    def __init__(self, chartData):
        self.chartData = chartData
        self.wedgeList = []

    # Sets up chart
    def setupChart(self):
        # Draw the pi charts for threads, thread specific library calls, thread-specific and library-specific function calls
        self.fig, self.axs = plt.subplots(1, figsize=(10, 10))
        #if "Kernel" in self.chartData[1]:
        #     del self.chartData[1]["Kernel"]
        self.wedgeList.append(self.axs.pie(self.chartData["funcs"].values(), labels=self.chartData["funcs"].keys(), autopct='%1.1f%%'))
        self.axs.set_title("Function Total On-CPU Execution Pi Chart")

        self.axs.axis('equal')

        self.axs.legend(self.chartData["funcs"].keys())

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
        # if self.figNum < 2:
        #    self.click = self.fig.canvas.mpl_connect('button_press_event', self.onclick)

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
        # if (self.figNum) < 2:
        #    self.fig.canvas.mpl_disconnect(self.click)

def main():
    with open("chart_data.json", 'r') as j_file:
        chartData = json.load(j_file)

    piChart = charter(chartData)
    piChart.setupChart()

    handler = wedgeHoverHandler(piChart, piChart.wedgeList[0][0], 0)

    plt.show()

    handler.disconnectHandlers()

if __name__ == "__main__":
    main()