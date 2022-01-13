<template>
  <v-container>
    <v-row v-for="elfItem in elfList" :key="elfItem.elfName">
      <v-col xl="12">
        <v-list subheader three-line>
          <v-subheader class="text-h4">{{ elfItem.elfName }}</v-subheader>

          <v-list-item
            v-for="elfConfigItem in elfItem.config"
            :key="elfConfigItem.key"
          >
            <v-list-item-content>
              <v-list-item-title>{{ elfConfigItem.key }}</v-list-item-title>
              <v-list-item-subtitle>{{
                elfConfigItem.value
              }}</v-list-item-subtitle>
            </v-list-item-content>
            <v-list-item-action v-if="elfConfigItem.extra">
              <v-btn
                icon
                @click="
                  showDialog(elfConfigItem.key, elfConfigItem.extra, true)
                "
              >
                <v-icon color="grey lighten-1">mdi-information</v-icon>
              </v-btn>
            </v-list-item-action>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col xl="12">
        <v-data-table
          :headers="elfSymInfoHeaders"
          :items="getELFTableData(elfItem.scalerId)"
          :single-expand="singleExpand"
          :expanded.sync="expanded"
          item-key="scalerId"
          show-expand
          class="elevation-1"
        >
        </v-data-table>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
// @ is an alias to /src
// import Welcome from "@/components/Welcome.vue";

export default {
  name: "",
  components: {
    // Welcome,
  },
  data() {
    return {
      expanded: [],
      singleExpand: false,

      elfSymInfoHeaders: [
        { text: "ScalerId", value: "scalerId" },
        { text: "Name", value: "symbolName" },
        { text: "Symbol Type", value: "symbolType" },
        { text: "Bind Type", value: "bindType" },
        { text: "Library Id", value: "libFileId" },
        { text: "Got addr", value: "gotAddr" },
        { text: "Hooked", value: "hooked" },
      ],
      desserts: [],
      elfList: [
        {
          elfName: "test.so",
          scalerId: 0,
          config: [
            { key: "File Name", value: "test.so" },
            { key: "File Path", value: "N/A" },
            { key: "Addr Start", value: "N/A" },
            { key: "Addr End", value: "N/A" },
            { key: ".plt Start Addr", value: "N/A" },
            { key: ".plt.sec Start Addr", value: "N/A" },
          ],
        },{
          elfName: "test1.so",
          scalerId: 1,
          config: [
            { key: "File Name", value: "test1.so" },
            { key: "File Path", value: "N/A" },
            { key: "Addr Start", value: "N/A" },
            { key: "Addr End", value: "N/A" },
            { key: ".plt Start Addr", value: "N/A" },
            { key: ".plt.sec Start Addr", value: "N/A" },
          ],
        },
      ],
      symbolList: [
        [
          {
            scalerId: "0",
            symbolName: "N/A",
            symbolType: "N/A",
            bindType: "N/A",
            callerFileId: "N/A",
            symIdInFile: "N/A",
            libFileId: "N/A",
            gotAddr: "N/A",
            hooked: "N/A",
          },
          {
            scalerId: "1",
            symbolName: "N/A",
            symbolType: "N/A",
            bindType: "N/A",
            callerFileId: "N/A",
            symIdInFile: "N/A",
            libFileId: "N/A",
            gotAddr: "N/A",
            hooked: "N/A",
          },
        ],
        [
          {
            scalerId: "0",
            symbolName: "N/A",
            symbolType: "N/A",
            bindType: "N/A",
            callerFileId: "N/A",
            symIdInFile: "N/A",
            libFileId: "N/A",
            gotAddr: "N/A",
            hooked: "N/A",
          },
          {
            scalerId: "1",
            symbolName: "N/A",
            symbolType: "N/A",
            bindType: "N/A",
            callerFileId: "N/A",
            symIdInFile: "N/A",
            libFileId: "N/A",
            gotAddr: "N/A",
            hooked: "N/A",
          },
        ]
      ],
    };
  },
  prop: {},
  model: {},
  methods: {
    showDialog: function (title, content, shouldShow) {
      this.dialogTitle = title;
      this.dialogText = content;
      this.isDialogShown = shouldShow;
    },
    getELFTableData: function (elfID) {
      console.log(this.symbolList[elfID]);
      return this.symbolList[elfID];
    },
  },
};
</script>
