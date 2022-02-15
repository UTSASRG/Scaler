<template>
  <v-container>
    <v-row>
      <v-col lg="6">
        <v-list subheader three-line>
          <v-subheader class="text-h4">Timing</v-subheader>
          <v-list-item>
            <v-list-item-content>
              <v-select
                label="Select visible ELF Image"
                multiple
                chips
                hint=""
                v-model="selectedELFImg"
                :items="timingELfImg"
                @change="updateTimingGraph()"
                item-text="baseName"
                :item-value="asdfwef"
                persistent-hint
              >
                <template v-slot:prepend-item>
                  <v-list-item
                    ripple
                    @mousedown.prevent
                    @click="selectAllELFFiles"
                  >
                    <v-list-item-content>
                      <v-list-item-title> Select All </v-list-item-title>
                    </v-list-item-content>
                  </v-list-item>
                  <v-divider class="mt-2"></v-divider>
                </template>
              </v-select>
            </v-list-item-content>
          </v-list-item>
          <v-list-item>
            <v-list-item-content>
              <v-text-field
                label="Visible symbol limit"
                v-model="visibleSymbolLimit"
              ></v-text-field>
            </v-list-item-content>
          </v-list-item>
          <v-list-item>
            <v-list-item-content>
              <v-select
                label="Select visible processes"
                multiple
                chips
                hint=""
                v-model="selectedProcesses"
                @change="updateTimingGraph()"
                :items="processIds"
                persistent-hint
              ></v-select>
            </v-list-item-content>
          </v-list-item>
          <v-list-item>
            <v-list-item-content>
              <v-select
                label="Select visible threads"
                multiple
                chips
                hint=""
                @change="updateTimingGraph()"
                v-model="selectedThreads"
                :items="threadIds"
                persistent-hint
              ></v-select>
            </v-list-item-content>
          </v-list-item>
          <v-list-item>
            <v-list-item-content>
              <v-checkbox
                v-model="bypassCommonSymbols"
                label="Bypass common symbols"
              ></v-checkbox>
            </v-list-item-content>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col lg="5">
        <v-chart
          class="chart"
          ref="applicationView"
          :option="applicationViewOption"
          @click="nodeClick"
          @finished="renderFinished"
        />
        <v-chart class="chart" ref="libraryView" :option="libraryViewOption" />
      </v-col>
    </v-row>
    <v-spacer></v-spacer>
  </v-container>
</template>

<script>
import path from "path";
import axios from "axios";
import { use } from "echarts/core";
import { CanvasRenderer } from "echarts/renderers";
import {
  PieChart,
  BarChart,
  SunburstChart,
  TreemapChart,
  TreeChart,
} from "echarts/charts";
import {
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  PolarComponent,
} from "echarts/components";
import VChart from "vue-echarts";
import { scalerConfig } from "@/scalerconfig.js";

use([
  CanvasRenderer,
  PieChart,
  TitleComponent,
  TooltipComponent,
  LegendComponent,
  PolarComponent,
  BarChart,
  SunburstChart,
  TreemapChart,
  TreeChart,
]);

export default {
  name: "TimingView",
  components: {
    VChart,
  },
  props: ["jobid"],
  data() {
    let _applicationViewData = [];
    let _applicationViewLabel = [];
    let _libraryViewData = [];
    let _libraryViewLabel = [];
    return {
      applicationViewOption: {
        title: {
          text: "Application View",
          show: false,
        },
        tooltip: {
          show: true,
          formatter: function (params) {
            let res = "";
            res += "Name : " + params.data.name + "</br>";
            res += "Cycles : " + params.data.value + "</br>";
            res += "Percent : " + params.data.percentage + "%</br>";

            return res;
          },
        },

        series: [
          {
            radius: ["15%", "80%"],
            type: "sunburst",
            sort: undefined,
            legend: {
              data: ["Invocation cycles"],
            },
            label: {
              show: true,
            },
            levels: [],
            data: _applicationViewData,
            categories: _applicationViewLabel,
          },
        ],
      },
      libraryViewOption: {
        title: {
          text: "Library View",
          show: false,
        },
        tooltip: {
          show: true,
          formatter: function (params) {
            let res = "";
            res += "Name : " + params.data.name + "</br>";
            res += "Cycles : " + params.data.value + "</br>";
            res += "Percent : " + params.data.percentage + "%</br>";

            return res;
          },
        },

        series: [
          {
            radius: ["15%", "80%"],
            type: "sunburst",
            sort: undefined,
            legend: {
              data: ["Invocation cycles"],
            },
            label: {
              show: true,
            },
            levels: [],
            data: _libraryViewData,
            categories: _libraryViewLabel,
          },
        ],
      },
      applicationViewData: _applicationViewData,
      applicationViewLabel: _applicationViewLabel,
      libraryViewData: _libraryViewData,
      libraryViewLabel: _libraryViewLabel,
      timingELfImg: [],
      threadList: [],
      selectedELFImg: null,
      selectedThreads: [],
      selectedProcesses: [],
      updatePieChartFlag: null,
      zoomToRootId: null,
      curRootId: null,
      visibleSymbolLimit: 20,
      threadIds: [],
      processIds: [],
      bypassCommonSymbols: true,
      bypassCommonSymbolList: ["pthread_join"],
    };
  },
  methods: {
    updateTimingGraph: function () {
      let thiz = this;
      var selectedIds = this.selectedELFImg.map((x) => x.id);

      axios
        .all([
          axios.post(
            scalerConfig.$ANALYZER_SERVER_URL +
              "/elfInfo/image/timing?jobid=" +
              thiz.jobid,
            {
              elfImgIds: selectedIds,
              visibleThreads: thiz.selectedThreads,
              visibleProcesses: thiz.selectedProcesses,
            }
          ),
          axios.post(
            scalerConfig.$ANALYZER_SERVER_URL +
              "/profiling/totalTime?jobid=" +
              thiz.jobid,
            {
              visibleThreads: thiz.selectedThreads,
              visibleProcesses: thiz.selectedProcesses,
            }
          ),
          axios.get(
            scalerConfig.$ANALYZER_SERVER_URL +
              "/profiling/totalTime/library?jobid=" +
              thiz.jobid
          ),
        ])
        .then(
          axios.spread((...responses) => {
            let responseTimingInfo = responses[0];
            let responseTotalTime = responses[1];
            console.log("TotalTime ", responseTotalTime.data);
            let responseLibraryTime = responses[2];

            var totalCycles = responseTotalTime.data;
            thiz.applicationViewData.splice(0);
            thiz.applicationViewLabel.splice(0);
            // var totalCyclesLibrary = 0;
            // for (let i = 0; i < thiz.selectedELFImg.length; i += 1) {
            //   totalCyclesLibrary += responseTimingInfo.data[i];
            // }
            // console.log(
            //   "totalCyclesLibrary is",
            //   totalCyclesLibrary,
            //   responseTimingInfo.data,
            //   totalCycles
            // );

            for (let i = 0; i < thiz.selectedELFImg.length; i += 1) {
              var curImg = thiz.selectedELFImg[i];
              thiz.applicationViewData.push({
                value: responseTimingInfo.data[i],
                name: path.basename(curImg.filePath),
                percentage: (
                  (responseTimingInfo.data[i] / totalCycles) *
                  100
                ).toFixed(2),
                imgObj: curImg,
                id: "" + curImg.id,
                children: [],
              });
              thiz.applicationViewLabel.push(curImg.filePath);
            }

            //todo: not efficient when there are lots of executables
            //ELFImage with scalerId 0 would always be the

            let mainElfImg = null;

            for (let i = 0; i < thiz.timingELfImg.length; ++i) {
              if (thiz.timingELfImg[i].scalerId == 0) {
                mainElfImg = thiz.timingELfImg[i];
                break;
              }
            }

            var selectedIds = this.selectedELFImg.map((x) => x.id);
            let totalCyclesLibrary = responseLibraryTime.data;
            if (selectedIds.includes(mainElfImg.id)) {
              //Adding main application
              thiz.applicationViewData.push({
                value: totalCycles - totalCyclesLibrary,
                name: path.basename(mainElfImg.filePath),
                percentage: (
                  ((totalCycles - totalCyclesLibrary) / totalCycles) *
                  100
                ).toFixed(2),
                imgObj: mainElfImg,
                id: "" + mainElfImg.id,
                children: [],
              });
              thiz.applicationViewLabel.push(mainElfImg.filePath);
            }

            if (thiz.applicationViewData.length > 0) {
              thiz.applicationViewOption.title.show = true;
            } else {
              thiz.applicationViewOption.title.show = false;
            }

            console.log(
              "Total time is",
              thiz.applicationViewData
            );
          })
        );

      // Fetching calling info for that specific image
    },
    asdfwef: function (Hello) {
      return Hello;
    },
    renderFinished: function () {
      if (this.zoomToRootId != null) {
        let _curRootID = this.zoomToRootId;
        this.zoomToRootId = null;
        let applicationView = this.$refs.applicationView;

        applicationView.dispatchAction({
          type: "sunburstRootToNode",
          targetNode: "" + _curRootID,
        });
      }
    },
    nodeClick: function (params) {
      if (params.treePathInfo.length == 2) {
        //First layer node licked
        let thiz = this;
        //Display detailed symbol sinfo
        if (
          this.applicationViewData.at(params.dataIndex - 1).children.length == 0
        ) {
          thiz.applicationViewOption.title.text =
            path.basename(params.data.imgObj.filePath) + " called";
          thiz.libraryViewOption.title.show = true;
          thiz.libraryViewOption.title.text =
            path.basename(params.data.imgObj.filePath) + " is called by";
          axios
            .get(
              scalerConfig.$ANALYZER_SERVER_URL +
                "/elfInfo/image/timing/symbols?jobid=" +
                thiz.jobid +
                "&elfImgId=" +
                params.data.imgObj.id +
                "&visibleSymbolLimit=" +
                thiz.visibleSymbolLimit +
                "&visibleThreads=" +
                thiz.selectedThreads +
                "&visibleProcesses=" +
                thiz.selectedProcesses
            )
            .then(function (responseSymInfo) {
              var parentSymNode = thiz.applicationViewData.at(
                params.dataIndex - 1
              );
              for (var i = 0; i < responseSymInfo.data.length; i += 1) {
                var curSymInfo = responseSymInfo.data[i];
                if (
                  thiz.bypassCommonSymbols &&
                  thiz.bypassCommonSymbolList.includes(curSymInfo.symbolName)
                ) {
                  continue;
                }

                thiz.applicationViewData
                  .at(params.dataIndex - 1)
                  .children.push({
                    value: curSymInfo.durations,
                    name: curSymInfo.symbolName,
                    percentage: (
                      (curSymInfo.durations / parentSymNode.value) *
                      100
                    ).toFixed(2),
                  });
              }
              thiz.zoomToRootId = params.data.id;
              thiz.curRootId = params.dataIndex - 1;
            });
        }
        //Display library view

        axios
          .post(
            scalerConfig.$ANALYZER_SERVER_URL +
              "/profiling/individualTime/library?jobid=" +
              thiz.jobid +
              "&elfImgId=" +
              params.data.imgObj.id +
              "&visibleThreads=" +
              thiz.selectedThreads +
              "&visibleProcesses=" +
              thiz.selectedProcesses,
            {
              visibleThreads: thiz.selectedThreads,
              visibleProcesses: thiz.selectedProcesses,
            }
          )
          .then(function (responseSymInfo) {
            thiz.libraryViewData.splice(0);
            let libExecTime = 0;
            for (let i = 0; i < responseSymInfo.data.length; i += 1) {
              let curLib = responseSymInfo.data[i];
              libExecTime += curLib.cycles;
            }

            for (let i = 0; i < responseSymInfo.data.length; i += 1) {
              let curLib = responseSymInfo.data[i];
              thiz.libraryViewData.push({
                value: curLib.cycles,
                name: path.basename(curLib.elfImgName),
                percentage: ((curLib.cycles / libExecTime) * 100).toFixed(2),
                children: [],
              });
              thiz.applicationViewLabel.push(curLib.elfImgName);
            }
          });
      } else if (params.treePathInfo.length == 1) {
        //Root node clicked
        this.libraryViewData.splice(0);
        this.applicationViewOption.title.text = "Time by components";
        this.libraryViewOption.title.show = false;

        if (this.curRootId != null) {
          this.applicationViewData.at(this.curRootId).children.splice(0);
          this.curRootId = null;
        }
      }
    },
    selectAllELFFiles: function () {
      this.selectedELFImg = this.timingELfImg;
      this.updateTimingGraph();
    },
  },
  mounted: function () {
    let thiz = this;
    //Get the number of symbols invoked by a elf image

    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/elfInfo/image?jobid=" +
          thiz.jobid +
          "&elfImgValid=true"
      )
      .then(function (responseImgInfo) {
        for (var i = 0; i < responseImgInfo.data.length; ++i) {
          var curImg = responseImgInfo.data[i];
          thiz.$set(curImg, "baseName", path.basename(curImg.filePath));
          thiz.timingELfImg.push(curImg);
        }
      });

    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/profiling/threads?jobid=" +
          thiz.jobid
      )
      .then(function (response) {
        thiz.threadIds = response.data;
      });

    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/profiling/processes?jobid=" +
          thiz.jobid
      )
      .then(function (response) {
        thiz.processIds = response.data;
      });
  },
};
</script>

<style scoped>
.chart {
  height: 400px;
  width: 400px;
}
</style>