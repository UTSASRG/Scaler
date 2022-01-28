<template>
  <v-container>
    <v-row>
      <v-col lg="6">
        <v-list subheader three-line>
          <v-subheader class="text-h4">Counting</v-subheader>
          <v-list-item>
            <v-list-item-content>
              <v-select
                label="Select visible ELF Image"
                multiple
                chips
                hint=""
                v-model="selectedELFImg"
                :items="countingELfImg"
                @change="updateCountingGraph"
                persistent-hint
              ></v-select>
            </v-list-item-content>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col lg="5">
        <v-chart class="chart" :option="sunburstOption" />
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
import { scalerConfig } from "../../../../scalerconfig.js";

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
  name: "HelloWorld",
  components: {
    VChart,
  },
  props: ["jobid",],
  data() {
    let countingData = [];

    return {
      sunburstOption: {
        series: [
          {
            radius: ["15%", "80%"],
            type: "sunburst",
            sort: undefined,
            emphasis: {
              focus: "ancestor",
            },
            label: {
              rotate: "radial",
            },
            levels: [],
            itemStyle: {
              color: "#ddd",
              borderWidth: 2,
            },
            data: countingData,
            countingData:countingData,
          },
        ],
      },
      countingELfImg: [],
      selectedELFImg:null
    };
  },
  methods:{
    updateCountingGraph:function(){
      console.log(this.selectedELFImg)
    }
  },
  mounted: function () {
    let thiz = this;
    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/elfInfo/image?jobid=" +
          thiz.jobid +
          "&elfImgValid=true"
      )
      .then(function (response) {
        console.log(response.data);
        for (var i = 0; i < response.data.length; ++i) {
          var curImg = response.data[i];
          thiz.countingELfImg.push(path.basename(curImg.filePath));
        }
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