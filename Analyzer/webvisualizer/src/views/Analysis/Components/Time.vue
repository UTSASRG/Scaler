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
                @change="updateCountingGraph()"
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
                value="20"
              ></v-text-field>
            </v-list-item-content>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col lg="5">
        <v-chart class="chart" :option="sunburstOption"/>
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
  props: ["jobid"],
  data() {
    let _countingData = [
     
    ];

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
            data: _countingData,
          },
        ],
      },
      countingData: _countingData,
      countingELfImg: [],
      selectedELFImg: null,
    };
  },
  methods: {
    updateCountingGraph: function () {
      let thiz = this;
      var selectedIds = this.selectedELFImg.map((x) => x.id);
      axios
        .post(
          scalerConfig.$ANALYZER_SERVER_URL +
            "/elfInfo/image/counting?jobid=" +
            thiz.jobid,
          { elfImgIds: selectedIds }
        )
        .then(function (responseCountingInfo) {
          thiz.countingData.splice(0)
          for(var i=0;i<thiz.selectedELFImg.length;i+=1){
            //var curImg=thiz.selectedELFImg[i];
            thiz.countingData.push({value:responseCountingInfo.data[i]})
          }
        });

      // Fetching calling info for that specific image
    },
    asdfwef: function (Hello) {
      return Hello;
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
        // console.log(responseImgInfo.data.map((elfImg) => elfImg.id))
        for (var i = 0; i < responseImgInfo.data.length; ++i) {
          var curImg = responseImgInfo.data[i];
          thiz.$set(curImg, "baseName", path.basename(curImg.filePath));
          thiz.countingELfImg.push(curImg);
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