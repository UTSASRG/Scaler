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
              ></v-select>
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
                v-model="selectedELFImg"
                @change="updateTimingGraph()"
                :items="selectedProcesses"
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
                v-model="selectedELFImg"
                :items="selectedThreads"
                persistent-hint
              ></v-select>
            </v-list-item-content>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col lg="5">
        <v-chart
          class="chart"
          ref="timingChart"
          :option="sunburstOption"
          @click="nodeClick"
          @finished="renderFinished"
        />
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
    let _timingData = [];
    let _timingLabel = [];
    return {
      sunburstOption: {
        tooltip: { show: true },

        series: [
          {
            radius: ["15%", "80%"],
            type: "sunburst",
            sort: undefined,
            legend: {
              data: ["Invocation cycles"],
            },
            label: {
              show: false,
            },
            levels: [],
            data: _timingData,
            categories: _timingLabel,
          },
        ],
      },
      timingData: _timingData,
      timingLabel: _timingLabel,
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
    };
  },
  methods: {
    updateTimingGraph: function () {
      let thiz = this;
      var selectedIds = this.selectedELFImg.map((x) => x.id);
      axios
        .post(
          scalerConfig.$ANALYZER_SERVER_URL +
            "/elfInfo/image/timing?jobid=" +
            thiz.jobid,
          {
            elfImgIds: selectedIds,
            visibleThreads: thiz.selectedThreads,
            visibleProcesses: thiz.selectedProcesses,
          }
        )
        .then(function (responseTimingInfo) {
          console.log(responseTimingInfo);
          thiz.timingData.splice(0);
          thiz.timingLabel.splice(0);
          for (var i = 0; i < thiz.selectedELFImg.length; i += 1) {
            var curImg = thiz.selectedELFImg[i];
            thiz.timingData.push({
              value: responseTimingInfo.data[i],
              name: curImg.filePath,
              imgObj: curImg,
              id: "" + curImg.id,
              children: [],
            });
            thiz.timingLabel.push(curImg.filePath);
          }
        });

      // Fetching calling info for that specific image
    },
    asdfwef: function (Hello) {
      return Hello;
    },
    renderFinished: function () {
      if (this.zoomToRootId != null) {
        let _curRootID = this.zoomToRootId;
        this.zoomToRootId = null;
        let timingChart = this.$refs.timingChart;

        timingChart.dispatchAction({
          type: "sunburstRootToNode",
          targetNode: "" + _curRootID,
        });
      }
    },
    nodeClick: function (params) {
      console.log("nodeclick");
      if (params.treePathInfo.length == 2) {
        let thiz = this;
        if (this.timingData.at(params.dataIndex - 1).children.length == 0) {
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
              //console.log(responseSymInfo)
              for (var i = 0; i < responseSymInfo.data.length; i += 1) {
                console.log(responseSymInfo.data[i]);
                thiz.timingData.at(params.dataIndex - 1).children.push({
                  value: responseSymInfo.data[i].durations,
                  name: responseSymInfo.data[i].symbolName,
                });
              }
              thiz.zoomToRootId = params.data.id;
              thiz.curRootId = params.dataIndex - 1;
              console.log(thiz.timingData);
            });
        }
      } else if (params.treePathInfo.length == 1) {
        //Root node clicked

        if (this.curRootId != null) {
          this.timingData.at(this.curRootId).children.splice(0);
          this.curRootId = null;
        }
      }
    },
  },
  mounted: function () {
    let thiz = this;
    //Get the number of symbols invoked by a elf image

    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL + "/elfInfo/image?jobid=" + thiz.jobid+
          "&elfImgValid=true"
      )
      .then(function (responseImgInfo) {
        // console.log(responseImgInfo.data.map((elfImg) => elfImg.id))
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
        // console.log(responseImgInfo.data.map((elfImg) => elfImg.id))
        thiz.selectedThreads = response.data;
      });

    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/profiling/processes?jobid=" +
          thiz.jobid
      )
      .then(function (response) {
        // console.log(responseImgInfo.data.map((elfImg) => elfImg.id))
        thiz.selectedProcesses = response.data;
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