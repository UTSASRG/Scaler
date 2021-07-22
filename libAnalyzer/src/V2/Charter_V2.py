import json
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
import numpy as np

class charter:
    def __init__(self, chartData):
        self.chartData = chartData
        self.wedgeList = []
        self.onThreadChart = True

    # Sets up chart
    def setupChart(self, key):
        # Draw the pi charts for threads, thread specific library calls, thread-specific and library-specific function calls
        self.fig, self.axs = plt.subplots(1, figsize=(10, 10))

        self.wedgeList.append(self.axs.pie(self.chartData[key].values(), labels=self.chartData[key].keys(), autopct='%1.1f%%', normalize=True))
        self.axs.set_title(f"Library total CPU execution time Pie Chart")

        self.axs.axis('equal')

        self.axs.legend(self.chartData[key].keys())

    def setupChartLibTotSamps(self):
        self.fig, self.axs = plt.subplots(1, figsize=(10,10))

        self.wedgeList.append(self.axs.pie(self.chartData["LibTotSamps"].values(), labels=self.chartData["LibTotSamps"].keys(), autopct='%1.1f%%'))
        self.axs.set_title("Library Samples")

        self.axs.axis('equal')

        self.axs.legend(self.chartData["LibTotSamps"].keys())


    def updateBack(self, event):
        if not self.onThreadChart:
            self.onThreadChart = True
            self.axs.clear()

            self.wedgeList[0] = self.axs.pie(self.chartData["ThreadSamps"].values(), labels=self.chartData["ThreadSamps"].keys(), autopct='%1.1f%%')
            self.axs.set_title("Thread Samples Pi Chart")

            self.axs.axis('equal')

            self.axs.legend(self.chartData["ThreadSamps"].keys())

            plt.draw()
        else:
            print("Did Nothing. Success.")
        return

    def updateChart(self, label):
        if self.onThreadChart:
            self.onThreadChart = False
            self.axs.clear()

            self.wedgeList[0] = self.axs.pie(self.chartData["LibSamps"][label].values(), labels=self.chartData["LibSamps"][label].keys(), autopct='%1.1f%%')
            self.axs.set_title(f"{label} Library Sample Totals")

            self.axs.axis('equal')

            self.axs.legend(self.chartData["LibSamps"][label].keys())

            plt.draw()
        return
# A wedge hover handler is made for each individual chart, so in the case of this charter,
# I have a pi chart for showing the threads
# A pi chart for the libraries of a selected thread
# A pi chart for the functions of a selected library of the selected thread
# Thus 3 handlers will be initialized
class wedgeHoverHandler:
    def __init__(self, chart, wedges):
        # Set attributes as a pi chart instance, figure number, and a pi chart's wedge list
        self.chart = chart
        self.wedges = wedges
        # Save the figure and axes we're drawing on to allow for the handler to update as needed
        self.fig = self.wedges[0].figure
        self.ax = self.wedges[0].axes
        #Set up an event handler
        self.motion = self.fig.canvas.mpl_connect('motion_notify_event', self.hover)
        self.click = self.fig.canvas.mpl_connect('button_press_event', self.onclick)

    #Function for exploding wedge on hover
    def hover(self, event):
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
    #For safely removing the event handlers at the end of execution
    def disconnectHandlers(self):
        self.fig.canvas.mpl_disconnect(self.motion)
        self.fig.canvas.mpl_disconnect(self.click)

def main():
    with open("chart_data.json", 'r') as j_file:
        chartData = json.load(j_file)

    Chart = charter(chartData)

    # Chart.setupChartLibTotSamps()

    Chart.setupChart("libs")

    # back = plt.axes([0.7, 0.05, 0.1, 0.075])
    # bback = Button(back, "Back", color='0.5', hovercolor='1.0')
    # bback.on_clicked(Chart.updateBack)

    handler = wedgeHoverHandler(Chart, Chart.wedgeList[0][0])

    plt.show()

    handler.disconnectHandlers()

if __name__ == "__main__":
    main()