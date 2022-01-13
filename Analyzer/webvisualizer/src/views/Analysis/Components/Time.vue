<template>
  <v-container>
    <v-row>
      <v-col lg="6">
        <v-list subheader three-line>
          <v-subheader class="text-h4">Settings</v-subheader>
          <v-list-item>
            <v-list-item-content>
              <v-list-item-title>Visibility Layers</v-list-item-title>
              <v-range-slider max="50" min="-50"></v-range-slider>
            </v-list-item-content>
          </v-list-item>
          <v-list-item>
            <v-list-item-content>
              <v-list-item-title>Visible Processes</v-list-item-title>
              <v-select :items="items"></v-select>
            </v-list-item-content>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col lg="5">
        <v-chart class="chart" :option="treeViewOption" />
      </v-col>
    </v-row>
    <v-spacer></v-spacer>
    <v-row>
      <v-chart class="chart" :option="treeMapOption" />
      <v-spacer></v-spacer>
      <v-chart class="chart" :option="sunburstOption" />
    </v-row>
    <v-spacer></v-spacer>
  </v-container>
</template>

<script>
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
  data() {
    const item1 = {
      color: "#F54F4A",
    };
    const item2 = {
      color: "#FF8C75",
    };
    const item3 = {
      color: "#FFB499",
    };
    const chartData = [
      {
        children: [
          {
            value: 5,
            children: [
              {
                value: 1,
                itemStyle: item1,
              },
              {
                value: 2,
                children: [
                  {
                    value: 1,
                    itemStyle: item2,
                  },
                ],
              },
              {
                children: [
                  {
                    value: 1,
                  },
                ],
              },
            ],
            itemStyle: item1,
          },
          {
            value: 10,
            children: [
              {
                value: 6,
                children: [
                  {
                    value: 1,
                    itemStyle: item1,
                  },
                  {
                    value: 1,
                  },
                  {
                    value: 1,
                    itemStyle: item2,
                  },
                  {
                    value: 1,
                  },
                ],
                itemStyle: item3,
              },
              {
                value: 2,
                children: [
                  {
                    value: 1,
                  },
                ],
                itemStyle: item3,
              },
              {
                children: [
                  {
                    value: 1,
                    itemStyle: item2,
                  },
                ],
              },
            ],
            itemStyle: item1,
          },
        ],
        itemStyle: item1,
      },
      {
        value: 9,
        children: [
          {
            value: 4,
            children: [
              {
                value: 2,
                itemStyle: item2,
              },
              {
                children: [
                  {
                    value: 1,
                    itemStyle: item1,
                  },
                ],
              },
            ],
            itemStyle: item1,
          },
          {
            children: [
              {
                value: 3,
                children: [
                  {
                    value: 1,
                  },
                  {
                    value: 1,
                    itemStyle: item2,
                  },
                ],
              },
            ],
            itemStyle: item3,
          },
        ],
        itemStyle: item2,
      },
      {
        value: 7,
        children: [
          {
            children: [
              {
                value: 1,
                itemStyle: item3,
              },
              {
                value: 3,
                children: [
                  {
                    value: 1,
                    itemStyle: item2,
                  },
                  {
                    value: 1,
                  },
                ],
                itemStyle: item2,
              },
              {
                value: 2,
                children: [
                  {
                    value: 1,
                  },
                  {
                    value: 1,
                    itemStyle: item1,
                  },
                ],
                itemStyle: item1,
              },
            ],
            itemStyle: item3,
          },
        ],
        itemStyle: item1,
      },
      {
        children: [
          {
            value: 6,
            children: [
              {
                value: 1,
                itemStyle: item2,
              },
              {
                value: 2,
                children: [
                  {
                    value: 2,
                    itemStyle: item2,
                  },
                ],
                itemStyle: item1,
              },
              {
                value: 1,
                itemStyle: item3,
              },
            ],
            itemStyle: item3,
          },
          {
            value: 3,
            children: [
              {
                value: 1,
              },
              {
                children: [
                  {
                    value: 1,
                    itemStyle: item2,
                  },
                ],
              },
              {
                value: 1,
              },
            ],
            itemStyle: item3,
          },
        ],
        itemStyle: item1,
      },
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
            data: chartData,
          },
        ],
      },
      treeMapOption: {
        series: [
          {
            name: "Disk Usage",
            type: "treemap",
            visibleMin: 300,
            label: {
              show: true,
              formatter: "{b}",
            },
            itemStyle: {
              borderColor: "#fff",
            },
            data: chartData,
          },
        ],
      },
      treeViewOption: {
        series: [
          {
            type: "tree",
            data: chartData,
            top: "1%",
            left: "7%",
            bottom: "1%",
            right: "20%",
            symbolSize: 7,
            label: {
              position: "left",
              verticalAlign: "middle",
              align: "right",
              fontSize: 9,
            },
            leaves: {
              label: {
                position: "right",
                verticalAlign: "middle",
                align: "left",
              },
            },
            emphasis: {
              focus: "descendant",
            },
            expandAndCollapse: true,
            animationDuration: 550,
            animationDurationUpdate: 750,
          },
        ],
      },
    };
  },
};
</script>

<style scoped>
.chart {
  height: 400px;
  width: 400px;
}
</style>